#include "ota.h"
#include "board_config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_http_client.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include <memory>
#include <algorithm>
#include <time.h>
// Arduino 2 also ships a same-named header for its own, separately initialized
// bundle. Use the ESP-IDF API linked in libmbedtls with the embedded default CA
// bundle; declaration matches the SDK esp_crt_bundle.h, avoiding header shadowing.
extern "C" esp_err_t esp_crt_bundle_attach(void*conf);
namespace Ota {namespace {
portMUX_TYPE mutex=portMUX_INITIALIZER_UNLOCKED;
Status shared;bool active=false;
struct Candidate {char version[24]={},url[256]={},hash[65]={};size_t size=0;uint32_t checked=0;};
Candidate candidate;
void publish(State s,unsigned p=0){portENTER_CRITICAL(&mutex);shared.state=s;shared.progress=p;portEXIT_CRITICAL(&mutex);}
void fail(Error e){Serial.printf("[OTA] error=%u\n",unsigned(e));portENTER_CRITICAL(&mutex);shared.state=State::Error;shared.error=e;portEXIT_CRITICAL(&mutex);}
struct Http {
 esp_http_client_handle_t handle=nullptr;char redirect[1536]={};
 ~Http(){if(handle)esp_http_client_cleanup(handle);}
 static esp_err_t event(esp_http_client_event_t*e){auto*self=static_cast<Http*>(e->user_data);if(e->event_id==HTTP_EVENT_ON_HEADER&&e->header_key&&e->header_value&&!strcasecmp(e->header_key,"Location"))strlcpy(self->redirect,e->header_value,sizeof self->redirect);return ESP_OK;}
 bool open(const char*url){String next=url;
  for(int hop=0;hop<5;hop++){
   if(!allowedUrl(next.c_str()))return false;if(handle){esp_http_client_cleanup(handle);handle=nullptr;}redirect[0]=0;
   esp_http_client_config_t c={};c.url=next.c_str();c.crt_bundle_attach=esp_crt_bundle_attach;c.disable_auto_redirect=true;c.timeout_ms=15000;c.buffer_size=4096;c.buffer_size_tx=2048;c.event_handler=event;c.user_data=this;
   handle=esp_http_client_init(&c);if(!handle)return false;
   esp_http_client_set_header(handle,"User-Agent","ESPRemote/1.3");esp_http_client_set_header(handle,"Accept-Encoding","identity");
   auto result=esp_http_client_open(handle,0);if(result!=ESP_OK){Serial.printf("[OTA] HTTPS open failed: %s\n",esp_err_to_name(result));return false;}
   if(esp_http_client_fetch_headers(handle)<0)return false;int code=esp_http_client_get_status_code(handle);
   if(code>=300&&code<400&&redirect[0]){next=redirect;continue;}if(code!=200){Serial.printf("[OTA] HTTP %d\n",code);return false;}return true;
  }return false;
 }
};
bool prerequisites(){if(WiFi.status()!=WL_CONNECTED){fail(Error::Network);return false;}if(time(nullptr)<1704067200){fail(Error::Clock);return false;}return true;}
void doCheck(){
 if(!prerequisites())return;Http http;if(!http.open(MANIFEST)){fail(Error::Http);return;}
 auto length=esp_http_client_get_content_length(http.handle);if(length>8192){fail(Error::Manifest);return;}
 std::unique_ptr<char[]> body(new(std::nothrow) char[8193]);if(!body){fail(Error::Memory);return;}
 size_t used=0;uint32_t started=millis();
 for(;;){
  if(used==8192||millis()-started>30000){fail(Error::Manifest);return;}
  int n=esp_http_client_read(http.handle,body.get()+used,8192-used);if(n<0||(!n&&!esp_http_client_is_complete_data_received(http.handle))){fail(Error::Download);return;}if(!n)break;used+=n;vTaskDelay(1);
 }
 body[used]=0;DynamicJsonDocument doc(12288);if(doc.capacity()==0){fail(Error::Memory);return;}if(deserializeJson(doc,body.get(),used)){fail(Error::Manifest);return;}
 const char*version=doc["version"]|"";const char*target=doc["target"]|"";const char*hash=doc["sha256"]|"";const char*url=doc["url"]|"";
 auto*partition=esp_ota_get_next_update_partition(nullptr);size_t size=doc["size"]|size_t(0);uint32_t parsed[3];
 String expected=String(RELEASE_PREFIX)+"download/v"+version+"/firmware.bin";
 if(doc["schema"].as<int>()!=1||strcmp(target,TARGET)||strlen(version)>=24||!versionParts(version,parsed)||!validHash(hash)||expected!=url||!partition||size<256||size>partition->size){fail(Error::Manifest);return;}
 Candidate selected;strlcpy(selected.version,version,sizeof selected.version);strlcpy(selected.url,url,sizeof selected.url);strlcpy(selected.hash,hash,sizeof selected.hash);selected.size=size;selected.checked=millis();
 Status result;result.state=newer(version,FIRMWARE_VERSION)?State::Available:State::Current;strlcpy(result.version,version,sizeof result.version);const char*langs[]={"pl","de","en"};for(int i=0;i<3;i++)strlcpy(result.notes[i],doc["notes"][langs[i]]|"",sizeof result.notes[i]);
 portENTER_CRITICAL(&mutex);candidate=selected;shared=result;portEXIT_CRITICAL(&mutex);
}
void doInstall(){
 if(!prerequisites())return;Candidate selected;portENTER_CRITICAL(&mutex);selected=candidate;portEXIT_CRITICAL(&mutex);
 if(!newer(selected.version,FIRMWARE_VERSION)||millis()-selected.checked>15*60*1000UL){fail(Error::Expired);return;}
 auto*partition=esp_ota_get_next_update_partition(nullptr);if(!partition||partition==esp_ota_get_running_partition()||selected.size>partition->size){fail(Error::Partition);return;}
 Http http;if(!http.open(selected.url)){fail(Error::Http);return;}if(esp_http_client_get_content_length(http.handle)!=int64_t(selected.size)){fail(Error::Download);return;}
 std::unique_ptr<uint8_t[]> buffer(new(std::nothrow) uint8_t[4096]);if(!buffer){fail(Error::Memory);return;}
 // Image + first segment header + application descriptor magic. This also
 // rejects merged USB images before writing. HTTP may split these bytes.
 size_t headerBytes=0;while(headerBytes<36){int n=esp_http_client_read(http.handle,(char*)buffer.get()+headerBytes,36-headerBytes);if(n<=0){fail(Error::Download);return;}headerBytes+=n;}
 if(!appHeader(buffer.get(),headerBytes)){fail(Error::Image);return;}
 esp_ota_handle_t output;if(esp_ota_begin(partition,selected.size,&output)!=ESP_OK){fail(Error::Write);return;}
 mbedtls_sha256_context hash;mbedtls_sha256_init(&hash);mbedtls_sha256_starts_ret(&hash,0);
 bool ok=esp_ota_write(output,buffer.get(),36)==ESP_OK&&mbedtls_sha256_update_ret(&hash,buffer.get(),36)==0;size_t done=36;uint32_t started=millis();
 while(ok&&done<selected.size){
  if(WiFi.status()!=WL_CONNECTED||millis()-started>600000){ok=false;break;}
  int n=esp_http_client_read(http.handle,(char*)buffer.get(),std::min(size_t(4096),selected.size-done));
  if(n<=0||esp_ota_write(output,buffer.get(),n)!=ESP_OK||mbedtls_sha256_update_ret(&hash,buffer.get(),n)!=0){ok=false;break;}
  done+=n;publish(State::Downloading,done*100/selected.size);vTaskDelay(1);
 }
 uint8_t digest[32];if(mbedtls_sha256_finish_ret(&hash,digest)!=0)ok=false;mbedtls_sha256_free(&hash);
 if(!ok||done!=selected.size||!esp_http_client_is_complete_data_received(http.handle)){esp_ota_abort(output);fail(Error::Download);return;}
 publish(State::Verifying,100);char hex[65];for(int i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",digest[i]);
 if(strcmp(hex,selected.hash)){esp_ota_abort(output);fail(Error::Checksum);return;}
 if(esp_ota_end(output)!=ESP_OK){fail(Error::Image);return;}
 if(esp_ota_set_boot_partition(partition)!=ESP_OK){fail(Error::Partition);return;}
 publish(State::Restarting,100);vTaskDelay(pdMS_TO_TICKS(1500));ESP.restart();
}
void worker(void*p){if(p)doInstall();else doCheck();portENTER_CRITICAL(&mutex);active=false;portEXIT_CRITICAL(&mutex);vTaskDelete(nullptr);}
bool start(bool install,const char*version){
 portENTER_CRITICAL(&mutex);if(active||(install&&(shared.state!=State::Available||!version||strcmp(version,candidate.version)))){portEXIT_CRITICAL(&mutex);return false;}
 active=true;shared.state=install?State::Downloading:State::Checking;shared.error=Error::None;shared.progress=0;
 if(!install){candidate=Candidate{};shared.version[0]=0;for(auto&n:shared.notes)n[0]=0;}portEXIT_CRITICAL(&mutex);
 if(xTaskCreatePinnedToCore(worker,"github-ota",16384,install?(void*)1:nullptr,1,nullptr,0)!=pdPASS){portENTER_CRITICAL(&mutex);active=false;portEXIT_CRITICAL(&mutex);fail(Error::Memory);return false;}return true;
}
}
Status status(){portENTER_CRITICAL(&mutex);Status result=shared;portEXIT_CRITICAL(&mutex);return result;}
bool busy(){portENTER_CRITICAL(&mutex);bool result=active;portEXIT_CRITICAL(&mutex);return result;}
bool installing(){return installing(status().state);}
bool check(){return start(false,nullptr);}
bool install(const char*v){return start(true,v);}
void reject(Error e){if(!busy())fail(e);}
}

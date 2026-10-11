#include "ota.h"
#include "board_config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_http_client.h>
#include <esp_tls.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include <memory>
#include <algorithm>
#include <time.h>
// Arduino 2 also ships a same-named header for its own, separately initialized
// bundle. Use the ESP-IDF API linked in libmbedtls with the embedded default CA
// bundle; declaration matches the SDK esp_crt_bundle.h, avoiding header shadowing.
extern "C" esp_err_t esp_crt_bundle_attach(void*conf);
extern "C" esp_err_t esp_crt_bundle_set(const uint8_t*bundle,size_t size);
extern const uint8_t otaRootsStart[] asm("_binary_certs_ota_roots_bin_start");
extern const uint8_t otaRootsEnd[] asm("_binary_certs_ota_roots_bin_end");
namespace Ota {namespace {
portMUX_TYPE mutex=portMUX_INITIALIZER_UNLOCKED;
Status shared;bool active=false;
struct Candidate {char version[24]={},url[256]={},hash[65]={};size_t size=0;uint32_t checked=0;};
Candidate candidate;
void publish(State s,unsigned p=0){portENTER_CRITICAL(&mutex);shared.state=s;shared.progress=p;portEXIT_CRITICAL(&mutex);}
void fail(Error e,int32_t detail=0){Serial.printf("[OTA] error=%u detail=%ld\n",unsigned(e),long(detail));portENTER_CRITICAL(&mutex);shared.state=State::Error;shared.error=e;shared.detail=detail;portEXIT_CRITICAL(&mutex);}
struct Http {
 esp_http_client_handle_t handle=nullptr;char redirect[2048]={};Error error=Error::Http;
 esp_err_t tlsError=ESP_OK;int tlsCode=0,tlsFlags=0;int32_t detail=0;bool truncated=false;
 ~Http(){if(handle)esp_http_client_cleanup(handle);}
 static esp_err_t event(esp_http_client_event_t*e){
  auto*self=static_cast<Http*>(e->user_data);
  if(e->event_id==HTTP_EVENT_ON_HEADER&&e->header_key&&e->header_value&&!strcasecmp(e->header_key,"Location"))self->truncated=strlcpy(self->redirect,e->header_value,sizeof self->redirect)>=sizeof self->redirect;
  if(e->event_id==HTTP_EVENT_ERROR&&e->data)self->tlsError=esp_tls_get_and_clear_last_error(static_cast<esp_tls_error_handle_t>(e->data),&self->tlsCode,&self->tlsFlags);
  return ESP_OK;
 }
 bool open(const char*url){String next=url;
  for(int hop=0;hop<5;hop++){
   if(!allowedUrl(next.c_str())){error=Error::Redirect;return false;}if(handle){esp_http_client_cleanup(handle);handle=nullptr;}redirect[0]=0;truncated=false;tlsError=ESP_OK;tlsCode=tlsFlags=0;
   // Log only host/phase; redirected asset URLs contain temporary access tokens.
   String host=next.substring(8,next.indexOf('/',8));Serial.printf("[OTA] hop=%d host=%s\n",hop,host.c_str());
   esp_http_client_config_t c={};c.url=next.c_str();c.crt_bundle_attach=esp_crt_bundle_attach;c.disable_auto_redirect=true;c.timeout_ms=20000;c.buffer_size=4096;c.buffer_size_tx=4096;c.event_handler=event;c.user_data=this;
   handle=esp_http_client_init(&c);if(!handle){error=Error::Memory;return false;}
   esp_http_client_set_header(handle,"User-Agent","ESPRemote/1.3");esp_http_client_set_header(handle,"Accept-Encoding","identity");
   auto result=esp_http_client_open(handle,0);if(result!=ESP_OK){
    Serial.printf("[OTA] open=%s tls=%s mbedtls=%d flags=0x%x\n",esp_err_to_name(result),esp_err_to_name(tlsError),tlsCode,tlsFlags);
    detail=tlsCode?tlsCode:(tlsError?tlsError:result);
    error=tlsError==ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME?Error::Dns:(tlsFlags||tlsCode||tlsError>=ESP_ERR_MBEDTLS_CERT_PARTLY_OK)?Error::Tls:Error::Connection;
    return false;
   }
   if(esp_http_client_fetch_headers(handle)<0){error=Error::Connection;Serial.println("[OTA] response headers interrupted/timeout");return false;}int code=esp_http_client_get_status_code(handle);
   Serial.printf("[OTA] HTTP=%d\n",code);
   if(code>=300&&code<400){if(!redirect[0]||truncated){error=Error::Redirect;return false;}next=redirect;continue;}
   if(code!=200){detail=code;error=code==404?Error::NotFound:(code==403||code==429)?Error::RateLimit:Error::Http;return false;}return true;
  }error=Error::Redirect;return false;
 }
};
bool prerequisites(){
 if(WiFi.status()!=WL_CONNECTED){fail(Error::Network);return false;}
 // The first check may arrive immediately after Wi-Fi association, before NTP.
 uint32_t started=millis();while(time(nullptr)<1704067200&&millis()-started<15000&&WiFi.status()==WL_CONNECTED)vTaskDelay(pdMS_TO_TICKS(100));
 if(WiFi.status()!=WL_CONNECTED){fail(Error::Network);return false;}if(time(nullptr)<1704067200){fail(Error::Clock);return false;}
 static bool rootsReady=false;if(!rootsReady){auto result=esp_crt_bundle_set(otaRootsStart,otaRootsEnd-otaRootsStart);if(result!=ESP_OK){fail(Error::Memory,result);return false;}rootsReady=true;Serial.println("[OTA] dedicated CA bundle ready (Mozilla 2025-02-25)");}
 return true;
}
void doCheck(){
 if(!prerequisites())return;Http http;if(!http.open(MANIFEST)){fail(http.error,http.detail);return;}
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
 Http http;if(!http.open(selected.url)){fail(http.error,http.detail);return;}if(esp_http_client_get_content_length(http.handle)!=int64_t(selected.size)){fail(Error::Download);return;}
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
 active=true;shared.state=install?State::Downloading:State::Checking;shared.error=Error::None;shared.progress=0;shared.detail=0;
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

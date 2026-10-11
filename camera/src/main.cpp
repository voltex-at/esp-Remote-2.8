#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <esp_camera.h>
#include <esp_http_server.h>
#include <img_converters.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include "camera_config.h"
static SemaphoreHandle_t cameraMutex,modelMutex;
static bool night=false,motionEnabled=true,noiseEnabled=true;
static uint32_t motionSeq=1,noiseSeq=1,lastMotion=0,lastNoise=0;
static uint8_t previous[192]={};static bool baseline=false;
static uint8_t* rgb=nullptr;static uint8_t movementStreak=0;
static bool audioReady=false;
static bool authorized(httpd_req_t*r){char token[80]={};if(httpd_req_get_hdr_value_str(r,"X-Pair-Token",token,sizeof token)!=ESP_OK)return false;size_t n=strlen(CAM_PAIR_TOKEN);if(strlen(token)!=n)return false;uint8_t diff=0;for(size_t i=0;i<n;i++)diff|=token[i]^CAM_PAIR_TOKEN[i];return diff==0;}
static esp_err_t reject(httpd_req_t*r){httpd_resp_set_status(r,"401 Unauthorized");return httpd_resp_send(r,"Pairing token required",HTTPD_RESP_USE_STRLEN);}
static void motion(camera_fb_t*fb){
 if(!motionEnabled||!rgb)return;if(!fmt2rgb888(fb->buf,fb->len,fb->format,rgb))return;
 uint32_t diff=0;for(int y=0;y<12;y++)for(int x=0;x<16;x++){int offset=((y*fb->height/12)*fb->width+(x*fb->width/16))*3;uint8_t v=((uint16_t)rgb[offset]+rgb[offset+1]+rgb[offset+2])/3;int i=y*16+x;diff+=abs((int)v-previous[i]);previous[i]=v;}
 if(baseline&&diff/192>18){if(++movementStreak>=2&&millis()-lastMotion>10000){xSemaphoreTake(modelMutex,portMAX_DELAY);motionSeq++;xSemaphoreGive(modelMutex);lastMotion=millis();movementStreak=0;}}else movementStreak=0;baseline=true;
}
static esp_err_t capture(httpd_req_t*r){
 if(!authorized(r))return reject(r);xSemaphoreTake(cameraMutex,portMAX_DELAY);camera_fb_t*fb=esp_camera_fb_get();if(!fb){xSemaphoreGive(cameraMutex);return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"Capture failed");}
 motion(fb);httpd_resp_set_type(r,"image/jpeg");httpd_resp_set_hdr(r,"Cache-Control","no-store");esp_err_t result=httpd_resp_send(r,(const char*)fb->buf,fb->len);esp_camera_fb_return(fb);xSemaphoreGive(cameraMutex);return result;
}
static esp_err_t status(httpd_req_t*r){
 if(!authorized(r))return reject(r);StaticJsonDocument<1024>d;d["protocol"]=1;d["name"]="Kamera 1";d["width"]=320;d["height"]=240;d["night"]=night;d["motion_enabled"]=motionEnabled;d["noise_enabled"]=noiseEnabled;
 xSemaphoreTake(modelMutex,portMAX_DELAY);d["motion_seq"]=motionSeq;d["noise_seq"]=noiseSeq;xSemaphoreGive(modelMutex);d["cap"]["audio"]=audioReady;d["cap"]["motion"]=rgb!=nullptr;d["cap"]["infrared"]=CAM_IR_PIN>=0;d["cap"]["cry_ai"]=false;
 char body[1024];size_t n=serializeJson(d,body,sizeof body);httpd_resp_set_type(r,"application/json");return httpd_resp_send(r,body,n);
}
static esp_err_t control(httpd_req_t*r){
 if(!authorized(r))return reject(r);char query[120]={},var[32]={},value[16]={};httpd_req_get_url_query_str(r,query,sizeof query);if(httpd_query_key_value(query,"var",var,sizeof var)!=ESP_OK||httpd_query_key_value(query,"val",value,sizeof value)!=ESP_OK)return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"var and val required");int v=atoi(value);
 xSemaphoreTake(cameraMutex,portMAX_DELAY);sensor_t*s=esp_camera_sensor_get();int result=0;
 if(!strcmp(var,"night")){night=v!=0;result=s->set_special_effect(s,night?2:0);s->set_gain_ctrl(s,1);s->set_exposure_ctrl(s,1);baseline=false;
 #if CAM_IR_PIN >= 0
 digitalWrite(CAM_IR_PIN,night?HIGH:LOW);
 #endif
 }else if(!strcmp(var,"motion")){motionEnabled=v!=0;baseline=false;}
 else if(!strcmp(var,"noise")){if(!audioReady)result=-1;else noiseEnabled=v!=0;}
 else result=-1;xSemaphoreGive(cameraMutex);if(result)return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Unsupported control");return httpd_resp_send(r,"OK",HTTPD_RESP_USE_STRLEN);
}
// Optional standard MJPEG endpoint on port 81. Token is passed as X-Pair-Token header.
static esp_err_t stream(httpd_req_t*r){
 if(!authorized(r))return reject(r);httpd_resp_set_type(r,"multipart/x-mixed-replace;boundary=frame");
 for(;;){xSemaphoreTake(cameraMutex,portMAX_DELAY);camera_fb_t*fb=esp_camera_fb_get();if(!fb){xSemaphoreGive(cameraMutex);return ESP_FAIL;}
 char head[100];int n=snprintf(head,sizeof head,"\r\n--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",(unsigned)fb->len);esp_err_t err=httpd_resp_send_chunk(r,head,n);if(err==ESP_OK)err=httpd_resp_send_chunk(r,(char*)fb->buf,fb->len);esp_camera_fb_return(fb);xSemaphoreGive(cameraMutex);if(err!=ESP_OK)return err;vTaskDelay(pdMS_TO_TICKS(125));}
}
#if CAM_AUDIO_ENABLED
static bool initAudio(){
 i2s_config_t c={};c.mode=(i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_RX|I2S_MODE_TX);c.sample_rate=16000;c.bits_per_sample=I2S_BITS_PER_SAMPLE_32BIT;c.channel_format=I2S_CHANNEL_FMT_ONLY_LEFT;c.communication_format=I2S_COMM_FORMAT_STAND_I2S;c.intr_alloc_flags=ESP_INTR_FLAG_LEVEL1;c.dma_buf_count=6;c.dma_buf_len=256;c.tx_desc_auto_clear=true;
 if(i2s_driver_install(I2S_NUM_1,&c,0,nullptr)!=ESP_OK)return false;i2s_pin_config_t p={};p.mck_io_num=-1;p.bck_io_num=CAM_AUDIO_BCLK;p.ws_io_num=CAM_AUDIO_LRCLK;p.data_in_num=CAM_AUDIO_MIC;p.data_out_num=CAM_AUDIO_SPEAKER;return i2s_set_pin(I2S_NUM_1,&p)==ESP_OK;
}
static void audioTask(void*){
 WiFiUDP udp;udp.begin(3333);IPAddress peer;uint16_t port=3333;uint32_t heard=0,talkUntil=0,seq=0;uint8_t packet[584]={};int32_t pcm32[256];int16_t pcm16[256];
 for(;;){int n=udp.parsePacket();if(n){IPAddress sender=udp.remoteIP();uint16_t senderPort=udp.remotePort();int got=udp.read(packet,sizeof packet);
 if((n==72||n==584)&&got==n&&!memcmp(packet,"BAP1",4)&&!strncmp((char*)packet+8,CAM_PAIR_TOKEN,64)){peer=sender;port=senderPort;heard=millis();if(n==584){memcpy(pcm16,packet+72,512);for(int i=0;i<256;i++)pcm32[i]=(int32_t)pcm16[i]*65536;size_t out;i2s_write(I2S_NUM_1,pcm32,sizeof pcm32,&out,pdMS_TO_TICKS(20));talkUntil=millis()+150;}}}
 size_t got=0;i2s_read(I2S_NUM_1,pcm32,sizeof pcm32,&got,pdMS_TO_TICKS(25));int count=got/4;double sum=0;
 for(int i=0;i<count;i++){pcm16[i]=constrain(pcm32[i]>>CAM_MIC_SHIFT,-32768,32767);sum+=(double)pcm16[i]*pcm16[i];}
 if(noiseEnabled&&count&&sqrt(sum/count)>CAM_NOISE_THRESHOLD&&millis()-lastNoise>10000){lastNoise=millis();xSemaphoreTake(modelMutex,portMAX_DELAY);noiseSeq++;xSemaphoreGive(modelMutex);}
 if(count==256&&millis()-heard<2000&&millis()>talkUntil){memcpy(packet,"BAP1",4);memcpy(packet+4,&seq,4);seq++;memset(packet+8,0,64);strlcpy((char*)packet+8,CAM_PAIR_TOKEN,64);memcpy(packet+72,pcm16,512);udp.beginPacket(peer,port);udp.write(packet,sizeof packet);udp.endPacket();}
 }
}
#endif
void setup(){
 Serial.begin(115200);cameraMutex=xSemaphoreCreateMutex();modelMutex=xSemaphoreCreateMutex();
 camera_config_t c={};c.ledc_channel=LEDC_CHANNEL_0;c.ledc_timer=LEDC_TIMER_0;c.pin_d0=CAM_D0;c.pin_d1=CAM_D1;c.pin_d2=CAM_D2;c.pin_d3=CAM_D3;c.pin_d4=CAM_D4;c.pin_d5=CAM_D5;c.pin_d6=CAM_D6;c.pin_d7=CAM_D7;c.pin_xclk=CAM_XCLK;c.pin_pclk=CAM_PCLK;c.pin_vsync=CAM_VSYNC;c.pin_href=CAM_HREF;c.pin_sccb_sda=CAM_SDA;c.pin_sccb_scl=CAM_SCL;c.pin_pwdn=CAM_PWDN;c.pin_reset=CAM_RESET;c.xclk_freq_hz=20000000;c.pixel_format=PIXFORMAT_JPEG;c.frame_size=FRAMESIZE_QVGA;c.jpeg_quality=14;c.fb_count=psramFound()?2:1;c.fb_location=psramFound()?CAMERA_FB_IN_PSRAM:CAMERA_FB_IN_DRAM;c.grab_mode=CAMERA_GRAB_LATEST;
 if(esp_camera_init(&c)!=ESP_OK){Serial.println("Camera init failed: check board/pin profile");return;}
 if(psramFound())rgb=(uint8_t*)ps_malloc(320*240*3);
 #if CAM_IR_PIN >= 0
 pinMode(CAM_IR_PIN,OUTPUT);digitalWrite(CAM_IR_PIN,LOW);
 #endif
 WiFi.mode(WIFI_AP_STA);WiFi.setSleep(false);WiFi.softAP(CAM_DIRECT_SSID,CAM_DIRECT_PASSWORD,6,0,2);if(strlen(CAM_ROUTER_SSID))WiFi.begin(CAM_ROUTER_SSID,CAM_ROUTER_PASSWORD);MDNS.begin("esp32-baby-cam");MDNS.addService("http","tcp",80);
 #if CAM_AUDIO_ENABLED
 audioReady=initAudio();if(audioReady)xTaskCreatePinnedToCore(audioTask,"camera-audio",6000,nullptr,3,nullptr,1);
 #endif
 httpd_config_t h=HTTPD_DEFAULT_CONFIG();h.stack_size=10000;h.max_uri_handlers=6;httpd_handle_t server=nullptr;if(httpd_start(&server,&h)==ESP_OK){httpd_uri_t u={};u.method=HTTP_GET;u.uri="/capture";u.handler=capture;httpd_register_uri_handler(server,&u);u.uri="/status";u.handler=status;httpd_register_uri_handler(server,&u);u.uri="/control";u.handler=control;httpd_register_uri_handler(server,&u);}
 h.server_port=81;h.ctrl_port+=1;httpd_handle_t mjpeg=nullptr;if(httpd_start(&mjpeg,&h)==ESP_OK){httpd_uri_t u={};u.uri="/stream";u.method=HTTP_GET;u.handler=stream;httpd_register_uri_handler(mjpeg,&u);}
 Serial.print("Direct camera IP: ");Serial.println(WiFi.softAPIP());
}
void loop(){static uint32_t last=0;if(WiFi.status()==WL_CONNECTED&&millis()-last>30000){last=millis();Serial.print("Router camera IP: ");Serial.println(WiFi.localIP());}delay(100);}

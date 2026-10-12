#include "runtime.h"
#include "portrait_display.h"
#include <driver/i2s.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <algorithm>
#include <esp_heap_caps.h>
static PortraitDisplay lcd;
static bool displayReady=false;
static char bootReason[96]={},bootDetail[96]={};
static lv_disp_draw_buf_t drawbuf;
static bool audioOk=false;
static char peerHost[64]={},peerToken[65]={};
static bool audioListen=false,audioTalk=false;
static SemaphoreHandle_t audioMutex;
static void flush(lv_disp_drv_t*d,const lv_area_t*a,lv_color_t*c){
 static_assert(sizeof(lv_color_t)==2&&LV_COLOR_16_SWAP==0,"SPI transport requires native RGB565");
 lcd.pushRect(a->x1,a->y1,a->x2-a->x1+1,a->y2-a->y1+1,(const uint16_t*)c);
 static bool first=true;if(first){first=false;Serial.printf("[LVGL] first flush: %d,%d to %d,%d\n",a->x1,a->y1,a->x2,a->y2);}
 lv_disp_flush_ready(d);
}
namespace Board {
void displayBegin(){
 pinMode(Pins::ampEnable,OUTPUT);digitalWrite(Pins::ampEnable,HIGH);
 lcd.begin();ledcSetup(0,5000,8);ledcAttachPin(Pins::backlight,0);brightness(80);displayReady=true;
 Serial.printf("[LCD] SPI initialization sent: %dx%d portrait, rotation=%u, MADCTL=0x%02X, IPS inversion ON\n",Portrait::width,Portrait::height,LCD_ROTATION,LCD_MADCTL);
}
static void paintBoot(const char*reason,const char*detail,bool error){
 if(!displayReady)return;lcd.fillScreen(error?0x2000:0x0842);lcd.setTextColor(0xffff);lcd.setTextSize(2);lcd.setCursor(12,26);lcd.print("ESP32 REMOTE");lcd.setTextSize(1);lcd.setCursor(12,54);lcd.print(FIRMWARE_VERSION);lcd.print(" / 240x320");
 lcd.drawRect(8,8,224,304,error?0xf800:0xafe5);lcd.setTextSize(1);lcd.setCursor(14,115);lcd.print(reason);lcd.setCursor(14,140);lcd.print(detail);lcd.setCursor(14,270);lcd.print(error?"USB log: 115200 baud":"Starting LVGL...");
}
void bootStatus(const char*reason,const char*detail){Serial.printf("[BOOT] %s: %s\n",reason,detail);paintBoot(reason,detail,false);}
void bootError(const char*reason,const char*detail){strlcpy(bootReason,reason,sizeof bootReason);strlcpy(bootDetail,detail,sizeof bootDetail);bootReport();paintBoot(reason,detail,true);}
void bootReport(){if(bootReason[0])Serial.printf("[BOOT ERROR] %s: %s\n",bootReason,bootDetail);}
bool begin(){
 if(!displayReady)displayBegin();
 Wire.begin(Pins::sda,Pins::scl,100000);Wire.setTimeOut(20);
 pinMode(Pins::touchReset,OUTPUT);digitalWrite(Pins::touchReset,LOW);delay(10);digitalWrite(Pins::touchReset,HIGH);delay(220);
 pinMode(Pins::touchIrq,INPUT_PULLUP);pinMode(Pins::boot,INPUT_PULLUP);pinMode(Pins::ampEnable,OUTPUT);digitalWrite(Pins::ampEnable,HIGH);
 auto* buf=(lv_color_t*)heap_caps_malloc(240*24*sizeof(lv_color_t),MALLOC_CAP_8BIT|MALLOC_CAP_INTERNAL);if(!buf){Serial.println("[LVGL] draw buffer allocation failed");return false;}
 lv_disp_draw_buf_init(&drawbuf,buf,nullptr,240*24);
 static lv_disp_drv_t disp;lv_disp_drv_init(&disp);disp.hor_res=240;disp.ver_res=320;disp.sw_rotate=1;disp.flush_cb=flush;disp.draw_buf=&drawbuf;if(!lv_disp_drv_register(&disp))return false;
 static lv_indev_drv_t touch;lv_indev_drv_init(&touch);touch.type=LV_INDEV_TYPE_POINTER;touch.read_cb=readTouch;if(!lv_indev_drv_register(&touch))return false;
 analogReadResolution(12);analogSetPinAttenuation(Pins::battery,ADC_11db);
 return true;
}
void brightness(uint8_t p){ledcWrite(0,(uint32_t)p*255/100);}
void readTouch(lv_indev_drv_t*,lv_indev_data_t*d){
 d->state=LV_INDEV_STATE_RELEASED;
 if(xSemaphoreTake(i2cMutex,0)!=pdTRUE)return;
 Wire.beginTransmission(0x38);Wire.write(0x02);
 if(Wire.endTransmission(false)==0 && Wire.requestFrom((uint8_t)0x38,(uint8_t)5)==5){
 uint8_t n=Wire.read();uint8_t xh=Wire.read(),xl=Wire.read(),yh=Wire.read(),yl=Wire.read();
 int16_t x,y;if((n&0x0f)>0 && (xh>>6)!=1&&Portrait::touch(((xh&0x0f)<<8)|xl,((yh&0x0f)<<8)|yl,x,y,LCD_ROTATION)){d->point.x=x;d->point.y=y;d->state=LV_INDEV_STATE_PRESSED;}
 }xSemaphoreGive(i2cMutex);
}
void battery(){
 uint32_t samples[16];for(auto&mv:samples)mv=analogReadMilliVolts(Pins::battery);std::sort(samples,samples+16);
 float v=((samples[7]+samples[8])/2000.0f)*BATTERY_DIVIDER*BATTERY_CALIBRATION;
 static Battery::Monitor monitor;auto reading=monitor.sample(v,millis());int pct=reading.percent;
 {Guard g(stateMutex);state.batteryV=reading.voltage;state.batteryPct=pct;state.batteryPower=reading.power;}
 static bool low=false;if(pct>=0&&pct<=15&&!low){low=true;alert("Niski poziom baterii");}if(pct>20)low=false;
}
// Register sequence adapted from Espressif ES8311 (Apache-2.0), 4.096MHz MCLK / 16kHz / 16-bit stereo slots.
static bool wr(uint8_t r,uint8_t v){Wire.beginTransmission(0x18);Wire.write(r);Wire.write(v);return Wire.endTransmission()==0;}
bool audioBegin(){
 audioMutex=xSemaphoreCreateMutex();
 if(!audioMutex)return false;
 i2s_config_t c={};c.mode=(i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_TX|I2S_MODE_RX);c.sample_rate=PCM_RATE;c.bits_per_sample=I2S_BITS_PER_SAMPLE_16BIT;c.channel_format=I2S_CHANNEL_FMT_RIGHT_LEFT;c.communication_format=I2S_COMM_FORMAT_STAND_I2S;c.intr_alloc_flags=ESP_INTR_FLAG_LEVEL1;c.dma_buf_count=6;c.dma_buf_len=256;c.use_apll=false;c.tx_desc_auto_clear=true;c.fixed_mclk=PCM_RATE*256;
 if(i2s_driver_install(I2S_NUM_0,&c,0,nullptr)!=ESP_OK)return false;
 i2s_pin_config_t p={};p.mck_io_num=Pins::mclk;p.bck_io_num=Pins::bclk;p.ws_io_num=Pins::lrclk;p.data_out_num=Pins::audioOut;p.data_in_num=Pins::audioIn;
 if(i2s_set_pin(I2S_NUM_0,&p)!=ESP_OK)return false;
 {Guard g(i2cMutex);if(!wr(0x00,0x1f))return false;}delay(20);
 bool ok=true;{Guard g(i2cMutex);
 const uint8_t init[][2]={{0x00,0x00},{0x01,0x30},{0x02,0x00},{0x03,0x10},{0x04,0x10},{0x05,0x00},{0x06,0x03},{0x07,0x00},{0x08,0xff},{0x09,0x0c},{0x0a,0x0c},{0x0b,0x00},{0x0c,0x00},{0x10,0x1f},{0x11,0x7f},{0x00,0x80},{0x0d,0x01},{0x01,0x3f},{0x14,0x1a},{0x12,0x00},{0x13,0x10},{0x0e,0x02},{0x0f,0x44},{0x15,0x00},{0x1b,0x0a},{0x1c,0x6a},{0x17,0xc8},{0x18,0x08},{0x32,0xc0},{0x37,0x08}};
 for(auto&r:init)ok=wr(r[0],r[1])&&ok;
 }audioOk=ok;return ok;
}
bool audioReady(){return audioOk;}
void audioSession(const char*h,const char*t,bool listen,bool talk){if(!audioMutex)return;Guard g(audioMutex);strlcpy(peerHost,h,sizeof peerHost);strlcpy(peerToken,t,sizeof peerToken);audioListen=listen;audioTalk=talk;}
static File wav;static uint32_t wavBytes=0;static char wavPath[96]={};
static void wavHeader(){uint8_t h[44]={};memcpy(h,"RIFF",4);uint32_t length=36+wavBytes;memcpy(h+4,&length,4);memcpy(h+8,"WAVEfmt ",8);uint32_t fmt=16,rate=PCM_RATE,bytes=PCM_RATE*2;uint16_t format=1,channels=1,align=2,bits=16;memcpy(h+16,&fmt,4);memcpy(h+20,&format,2);memcpy(h+22,&channels,2);memcpy(h+24,&rate,4);memcpy(h+28,&bytes,4);memcpy(h+32,&align,2);memcpy(h+34,&bits,2);memcpy(h+36,"data",4);memcpy(h+40,&wavBytes,4);wav.seek(0);wav.write(h,44);wav.seek(44+wavBytes);}
static void closeWav(){if(wav){Guard g(sdMutex);wavHeader();wav.flush();wav.close();}wavPath[0]=0;}
static void recordAudio(const AppState&s,const int16_t*pcm){
 if(!s.recording)return;Guard g(sdMutex);if(!Storage::available())return;
 if(!wav){String path=s.clipPath;path.replace(".rvc",".wav");strlcpy(wavPath,s.clipPath,sizeof wavPath);wav=SD_MMC.open(path,FILE_WRITE);wavBytes=0;if(!wav)return;wavHeader();}
 // Keep the audio timeline aligned with the clip's monotonic start. Missing UDP packets become silence.
 uint32_t target=(uint64_t)(millis()-s.clipStarted)*PCM_RATE/1000*2;uint8_t zeros[512]={};
 while(target>wavBytes+1024){size_t n=std::min((uint32_t)sizeof zeros,target-wavBytes-512);size_t wrote=wav.write(zeros,n);wavBytes+=wrote;if(wrote!=n)return;}
 size_t wrote=wav.write((const uint8_t*)pcm,512);wavBytes+=wrote;if((wavBytes/512)%64==0){wavHeader();wav.flush();}
}
// Fixed packet: 4-byte magic BAP1, seq u32 LE, 64-byte token (NUL padded), 256 x PCM s16 LE.
void audioTask(void*){
 if(!audioMutex){vTaskDelete(nullptr);return;}
 AppState*local=allocateStateCopy();if(!local){message("Brak PSRAM dla audio");vTaskDelete(nullptr);return;}AppState&status=*local;
 WiFiUDP udp;udp.begin(AUDIO_UDP_PORT);uint8_t packet[584]={};memcpy(packet,"BAP1",4);uint32_t seq=0,heartbeat=0;
 int16_t stereo[512],mono[256];IPAddress peer;
 for(;;){
 copyState(status);if(wav&&(!status.recording||strcmp(wavPath,status.clipPath)))closeWav();
 bool listen,talk;char host[64],token[65];{Guard g(audioMutex);listen=audioListen;talk=audioTalk;strlcpy(host,peerHost,sizeof host);strlcpy(token,peerToken,sizeof token);}
 bool active=audioOk&&(listen||talk)&&WiFi.isConnected()&&(peer.fromString(host)||WiFi.hostByName(host,peer)==1);
 digitalWrite(Pins::ampEnable,active&&listen&&!talk?LOW:HIGH);
 if(!active){vTaskDelay(pdMS_TO_TICKS(50));continue;}
 size_t got=0;i2s_read(I2S_NUM_0,stereo,sizeof stereo,&got,pdMS_TO_TICKS(20));
 int count=got/4;double sum=0;for(int i=0;i<count;i++){mono[i]=stereo[i*2];sum+=(double)mono[i]*mono[i];}
 {Guard g(stateMutex);state.audioRms=count?sqrt(sum/count):0;}
 if((talk&&count==256)||millis()-heartbeat>500){heartbeat=millis();memcpy(packet+4,&seq,4);seq++;memset(packet+8,0,64);strlcpy((char*)packet+8,token,64);if(talk&&count==256)memcpy(packet+72,mono,512);udp.beginPacket(peer,AUDIO_UDP_PORT);udp.write(packet,talk&&count==256?584:72);udp.endPacket();}
 int n=udp.parsePacket();if(n){IPAddress sender=udp.remoteIP();int read=udp.read(packet,sizeof packet);if(sender==peer&&n==584&&read==584&&!memcmp(packet,"BAP1",4)&&!strncmp((char*)packet+8,token,64)&&listen&&!talk){uint8_t vol=status.settings.volume;memcpy(mono,packet+72,512);recordAudio(status,mono);for(int i=0;i<256;i++)stereo[i*2]=stereo[i*2+1]=(int32_t)mono[i]*vol/100;size_t out;i2s_write(I2S_NUM_0,stereo,sizeof stereo,&out,pdMS_TO_TICKS(20));}}
 }
}
}

#include "runtime.h"
#include "ui.h"
#include "wifi_networks.h"
#include "ota.h"
#include "tls_memory.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <ArduinoOTA.h>
#include <time.h>
SET_LOOP_TASK_STACK_SIZE(16384);
AppState state;
SemaphoreHandle_t stateMutex,i2cMutex,sdMutex,frameMutex;
QueueHandle_t commands,mediaCommands;
uint16_t* videoPixels=nullptr;bool videoDirty=false;
static Preferences config;static bool ready=false;
static QueueHandle_t networkCommands;
// Retained across software/panic/watchdog resets, without writing flash.
// A video cannot distinguish a watchdog, a panic and a supply brownout;
// record the reset cause and the last completed startup stage for USB logs.
enum class BootStage:uint8_t { Start,Lcd,Config,Lvgl,Sd,Audio,Sensors,Network,Media,AudioTask,Running };
RTC_NOINIT_ATTR static volatile BootStage retainedStage;
RTC_NOINIT_ATTR static volatile uint32_t retainedMagic;
constexpr uint32_t BOOT_TRACE_MAGIC=0x45535231;
static const char*stageName(BootStage s){switch(s){
 case BootStage::Lcd:return "LCD";case BootStage::Config:return "config/PSRAM";case BootStage::Lvgl:return "LVGL";
 case BootStage::Sd:return "SD";case BootStage::Audio:return "audio init";case BootStage::Sensors:return "sensors task";
 case BootStage::Network:return "network task";case BootStage::Media:return "media task";case BootStage::AudioTask:return "audio task";
 case BootStage::Running:return "running";default:return "start";
}}
static const char*resetName(esp_reset_reason_t r){switch(r){
 case ESP_RST_POWERON:return "power-on";case ESP_RST_EXT:return "external";case ESP_RST_SW:return "software";
 case ESP_RST_PANIC:return "panic";case ESP_RST_INT_WDT:return "interrupt watchdog";case ESP_RST_TASK_WDT:return "task watchdog";
 case ESP_RST_WDT:return "watchdog";case ESP_RST_DEEPSLEEP:return "deep sleep";case ESP_RST_BROWNOUT:return "brownout";
 case ESP_RST_SDIO:return "SDIO";default:return "unknown";
}}
static void stage(BootStage s){retainedStage=s;retainedMagic=BOOT_TRACE_MAGIC;Serial.printf("[BOOT STAGE] %s ms=%lu internal=%u stack=%u\n",stageName(s),(unsigned long)millis(),ESP.getFreeHeap(),(unsigned)uxTaskGetStackHighWaterMark(nullptr));}
AppState snapshot(){Guard g(stateMutex);return state;}
void message(const char*t){Guard g(stateMutex);strlcpy(state.message,t,sizeof state.message);}
void alert(const char*t){
 {Guard g(stateMutex);if(state.alertCount<ALERT_COUNT)state.alertCount++;for(int i=ALERT_COUNT-1;i>0;i--)strlcpy(state.alerts[i],state.alerts[i-1],96);snprintf(state.alerts[0],96,"%s  %s",state.timeText,t);if(state.settings.notifications)strlcpy(state.message,t,sizeof state.message);}
 Storage::logEvent(t);
}
void dispatch(Action a,int value,const char*t){Command c={a,value,{0}};if(t)strlcpy(c.text,t,sizeof c.text);if(xQueueSend(commands,&c,0)!=pdTRUE)message("Kolejka polecen pelna");}
static void load(){config.begin("remote-v1",false);Settings defaults;Settings s=defaults;
 // Versioned POD blob, no sensitive values printed to serial.
 if(config.getUInt("schema",0)==1&&config.getBytesLength("settings")==sizeof s)config.getBytes("settings",&s,sizeof s);
 s.ssid[32]=0;s.password[64]=0;s.cameraHost[63]=0;s.pairToken[64]=0;s.brightness=constrain(s.brightness,5,100);s.language=constrain(s.language,0,2);s.volume=constrain(s.volume,0,100);s.timeoutSec=constrain(s.timeoutSec,15,600);s.logIntervalSec=constrain(s.logIntervalSec,5,300);
 Guard g(stateMutex);state.settings=s;
}
static void save(){Settings s=snapshot().settings;config.putBytes("settings",&s,sizeof s);config.putUInt("schema",1);}
static Settings networkSettings(){Guard g(stateMutex);return state.settings;}
static bool connect(){Settings s=networkSettings();WiFi.disconnect();WiFi.mode(WIFI_STA);WiFi.setSleep(false);WiFi.setAutoReconnect(true);
 if(!s.ssid[0]){Guard g(stateMutex);state.wifiLink=WifiLinkState::Offline;return false;}
 WiFi.begin(s.ssid,s.password);
 {Guard g(stateMutex);state.wifiLink=WifiLinkState::Connecting;}return true;}
static void apply(int key,const char*text,int value){
 {Guard g(stateMutex);auto&s=state.settings;switch(key){
 case Brightness:s.brightness=constrain(value,5,100);break;case Timeout:s.timeoutSec=constrain(value,15,600);break;case Language:s.language=constrain(value,0,2);break;case LogInterval:s.logIntervalSec=constrain(value,5,300);break;case Logging:s.logging=value;break;case Notifications:s.notifications=value;break;case Volume:s.volume=constrain(value,0,100);break;case NetworkMode:s.networkMode=value?1:0;if(!s.networkMode)strlcpy(s.cameraHost,"192.168.4.1",sizeof s.cameraHost);break;
 case WifiSsid:strlcpy(s.ssid,text,sizeof s.ssid);break;case WifiPassword:strlcpy(s.password,text,sizeof s.password);break;case CameraHost:if(strstr(text,"://")||strchr(text,'/')){strlcpy(state.message,"Podaj tylko IP lub nazwe hosta",sizeof state.message);return;}strlcpy(s.cameraHost,text,sizeof s.cameraHost);break;case PairToken:if(strlen(text)>63){strlcpy(state.message,"Token: maksymalnie 63 znaki",sizeof state.message);return;}strlcpy(s.pairToken,text,sizeof s.pairToken);break;
 case TempMin:s.tempMin=value;break;case TempMax:s.tempMax=value;break;case HumidityMin:s.humidityMin=constrain(value,0,100);break;case HumidityMax:s.humidityMax=constrain(value,0,100);break;case Co2Max:s.co2Max=constrain(value,400,5000);break;case IaqMax:s.iaqMax=constrain(value,25,500);break;
 }}save();Board::brightness(snapshot().settings.brightness);
}
static WifiSecurity security(wifi_auth_mode_t auth){switch(auth){
 case WIFI_AUTH_OPEN:return WifiSecurity::Open;
 case WIFI_AUTH_WPA_PSK:return WifiSecurity::Wpa;
 case WIFI_AUTH_WPA2_PSK:return WifiSecurity::Wpa2;
 case WIFI_AUTH_WPA3_PSK:return WifiSecurity::Wpa3;
 case WIFI_AUTH_WPA_WPA2_PSK:return WifiSecurity::WpaWpa2;
 case WIFI_AUTH_WPA2_WPA3_PSK:return WifiSecurity::Wpa2Wpa3;
 default:return WifiSecurity::Unsupported;
}}
static void publishScan(int result){
 WifiNetwork list[WIFI_NETWORK_COUNT];uint8_t count=0;
 if(result>=0)for(int i=0;i<result;i++){
  auto*ap=(wifi_ap_record_t*)WiFiScanClass::getScanInfoByIndex(i);if(!ap)continue;
  WifiNetwork n;memcpy(n.ssid,ap->ssid,32);n.ssid[32]=0;n.rssi=ap->rssi;n.channel=ap->primary;n.security=security(ap->authmode);addWifiNetwork(list,count,n);
 }
 {Guard g(stateMutex);memcpy(state.networks,list,sizeof list);state.networkCount=count;state.wifiScan=result>=0?WifiScanState::Done:WifiScanState::Failed;state.wifiScanGeneration++;}
 WiFi.scanDelete();WiFi.setAutoReconnect(true);
}
static void networkTask(void*){
 // All Wi-Fi connect/scan calls run here, never in the LVGL task.
 bool was=false,ota=false,scanPending=false,scanning=false,joinPending=false;
 uint32_t retry=millis(),attempt=retry,scanReady=0;bool attempting=false;WiFi.persistent(false);attempting=connect();
 for(;;){uint32_t now=millis();Command c;
  if(Ota::busy()){vTaskDelay(pdMS_TO_TICKS(50));continue;}
  while(xQueueReceive(networkCommands,&c,0)==pdTRUE){
   if(c.action==Action::Connect)joinPending=true;
   if(c.action==Action::WifiScan&&!scanning&&!scanPending){
    scanPending=true;scanReady=now+200;WiFi.setAutoReconnect(false);
    // The ESP-IDF driver rejects scans while attempting an association.
    // Pause an unsuccessful attempt; an existing connection stays active.
    bool offline=WiFi.status()!=WL_CONNECTED;if(offline){WiFi.disconnect(false,false);attempting=false;}
    Guard g(stateMutex);state.wifiScan=WifiScanState::Scanning;state.wifiScanGeneration++;if(offline)state.wifiLink=WifiLinkState::Offline;
   }
  }
  if(scanPending&&(int32_t)(now-scanReady)>=0){
   scanPending=false;int result=WiFi.scanNetworks(true,false,false,300);
   scanning=result==WIFI_SCAN_RUNNING;
   if(!scanning){publishScan(result);joinPending=joinPending||WiFi.status()!=WL_CONNECTED;}
  }
  if(scanning){int result=WiFi.scanComplete();if(result!=WIFI_SCAN_RUNNING){
   if(result<0)esp_wifi_scan_stop();scanning=false;publishScan(result);joinPending=joinPending||WiFi.status()!=WL_CONNECTED;
  }}
  if(joinPending&&!scanning&&!scanPending){attempting=connect();joinPending=false;retry=attempt=now;}
  bool connected=WiFi.status()==WL_CONNECTED;
  if(connected)attempting=false;
  bool failed=!connected&&attempting&&now-attempt>=15000&&!scanning&&!scanPending;
  if(failed){WiFi.disconnect(false,false);attempting=false;}
  {Guard g(stateMutex);state.wifi=connected;state.rssi=connected?WiFi.RSSI():-127;
   strlcpy(state.ip,connected?WiFi.localIP().toString().c_str():"0.0.0.0",sizeof state.ip);
   strlcpy(state.connectedSsid,connected?WiFi.SSID().c_str():"",sizeof state.connectedSsid);
   if(connected)state.wifiLink=WifiLinkState::Connected;
   else if(failed)state.wifiLink=WifiLinkState::Failed;
   else if(attempting)state.wifiLink=WifiLinkState::Connecting;
   else if(was)state.wifiLink=WifiLinkState::Offline;
  }
  if(connected&&!was){configTzTime("CET-1CEST,M3.5.0,M10.5.0/3","pool.ntp.org","time.nist.gov");Settings s=networkSettings();if(s.pairToken[0]){ArduinoOTA.setHostname("esp32-remote");ArduinoOTA.setPassword(s.pairToken);ArduinoOTA.begin();ota=true;}else{ArduinoOTA.end();ota=false;}}
  if(!connected&&was)alert("Utracono polaczenie Wi-Fi");if(connected&&ota&&!Ota::busy())ArduinoOTA.handle();
  if(!connected&&!attempting&&!scanning&&!scanPending&&now-retry>30000){joinPending=true;}
  was=connected;vTaskDelay(pdMS_TO_TICKS(50));
 }
}
void setup(){
 Serial.begin(115200);uint32_t usbWait=millis();while(!Serial&&millis()-usbWait<250)delay(5);
 const esp_reset_reason_t reset=esp_reset_reason();
 const BootStage previous=reset!=ESP_RST_POWERON&&retainedMagic==BOOT_TRACE_MAGIC&&(uint8_t)retainedStage<=(uint8_t)BootStage::Running?retainedStage:BootStage::Start;
 Serial.printf("[BOOT RESET] reason=%s (%d) previous=%s\n",resetName(reset),(int)reset,stageName(previous));
 stage(BootStage::Start);
 // LCD must be alive before any PSRAM/LVGL allocation can abort startup.
 stage(BootStage::Lcd);
 Board::displayBegin();Board::bootStatus("Hardware startup","LCD initialized before PSRAM");
 Serial.printf("[BOOT] firmware=%s reset=%d PSRAM=%u free=%u internal=%u\n",FIRMWARE_VERSION,(int)esp_reset_reason(),ESP.getPsramSize(),ESP.getFreePsram(),ESP.getFreeHeap());
 stateMutex=xSemaphoreCreateMutex();i2cMutex=xSemaphoreCreateMutex();sdMutex=xSemaphoreCreateMutex();frameMutex=xSemaphoreCreateMutex();commands=xQueueCreate(16,sizeof(Command));mediaCommands=xQueueCreate(16,sizeof(Command));networkCommands=xQueueCreate(8,sizeof(Command));
 if(!stateMutex||!i2cMutex||!sdMutex||!frameMutex||!commands||!mediaCommands||!networkCommands){Board::bootError("RTOS allocation failed","Check internal RAM / USB log");return;}
 stage(BootStage::Config);load();if(!psramFound()){Board::bootError("PSRAM not found","Check N16R8 / qio_opi");return;}
 // Prove the custom LVGL allocator works, rather than letting lv_init assert.
 void*probe=heap_caps_malloc(256*1024,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!probe){Board::bootError("PSRAM allocation failed","LVGL needs external RAM");return;}heap_caps_free(probe);
 if(!TlsMemory::begin()){Board::bootError("TLS allocator init failed","Check PSRAM / USB log");return;}
 stage(BootStage::Lvgl);Board::bootStatus("LVGL startup","RGB565 / native portrait");lv_init();if(!Board::begin()){Board::bootError("LVGL display allocation failed","Read the USB startup log");return;}
 videoPixels=(uint16_t*)ps_calloc(224*168,2);if(!videoPixels){message("Wymagana PSRAM OPI 8MB");}
 Ui::begin(dispatch);Ui::update(snapshot());Board::brightness(snapshot().settings.brightness);lv_refr_now(nullptr);
 stage(BootStage::Sd);bool sd=Storage::begin();{Guard g(stateMutex);state.sd=sd;}
 stage(BootStage::Audio);bool audio=Board::audioBegin();Serial.printf("[BOOT] SD=%d audio=%d\n",sd,audio);
 stage(BootStage::Sensors);bool tasks=xTaskCreatePinnedToCore(Sensors::task,"sensors",16000,nullptr,2,nullptr,0)==pdPASS;
 stage(BootStage::Network);tasks=(xTaskCreatePinnedToCore(networkTask,"network",16000,nullptr,1,nullptr,0)==pdPASS)&&tasks;
 stage(BootStage::Media);if(videoPixels)tasks=(xTaskCreatePinnedToCore(Media::task,"media",16000,nullptr,1,nullptr,0)==pdPASS)&&tasks;
 stage(BootStage::AudioTask);tasks=(xTaskCreatePinnedToCore(Board::audioTask,"audio",12000,nullptr,3,nullptr,0)==pdPASS)&&tasks;
 if(!tasks){Board::bootError("Task allocation failed","Check free internal RAM");return;}
 if(reset==ESP_RST_PANIC||reset==ESP_RST_INT_WDT||reset==ESP_RST_TASK_WDT||reset==ESP_RST_WDT||reset==ESP_RST_BROWNOUT){
  char text[120];snprintf(text,sizeof text,"Restart: %s (%s). USB: 115200",resetName(reset),stageName(previous));message(text);
 }
 ready=true;stage(BootStage::Running);Serial.println("[BOOT] READY: LVGL + sensors + network");
}
void loop(){
 if(!ready){static uint32_t report=0;if(millis()-report>2000){report=millis();Board::bootReport();}delay(100);return;}
 static uint32_t tick=millis(),uiTick=0,batteryTick=0,saveDue=0;uint32_t now=millis();lv_tick_inc(now-tick);tick=now;
 Command c;while(xQueueReceive(commands,&c,0)==pdTRUE){
 if(Ota::installing())continue;
 switch(c.action){
 case Action::OtaCheck:case Action::OtaInstall:{
  auto s=snapshot();if(s.recording||s.playing||s.listening||s.talking||s.wifiScan==WifiScanState::Scanning){Ota::reject(Ota::Error::Busy);break;}
  if(c.action==Action::OtaInstall&&s.batteryPct>=0&&s.batteryPct<=15){Ota::reject(Ota::Error::Battery);break;}
  if(c.action==Action::OtaCheck)Ota::check();else Ota::install(c.text);break;
 }
 case Action::Setting:apply(c.value,c.text,atoi(c.text));saveDue=now;break;
 case Action::Scan:Sensors::scan();break;
 case Action::Connect:case Action::WifiScan:if(xQueueSend(networkCommands,&c,0)!=pdTRUE)message("Kolejka Wi-Fi pelna");break;
 case Action::Reset:config.clear();{Guard g(stateMutex);state.settings=Settings();}save();ESP.restart();break;
 case Action::ClearAlerts:{Guard g(stateMutex);state.alertCount=0;}break;
 case Action::Reboot:ESP.restart();break;
 default:if(xQueueSend(mediaCommands,&c,0)!=pdTRUE)message("Kolejka kamery pelna");break;
 }}(void)saveDue;
 if(now-batteryTick>=2000){batteryTick=now;Board::battery();struct tm tm;if(getLocalTime(&tm,0)){Guard g(stateMutex);strftime(state.timeText,sizeof state.timeText,"%H:%M",&tm);} uint64_t freeBytes=0;{Guard sd(sdMutex);if(snapshot().sd){uint64_t total=SD_MMC.totalBytes(),used=SD_MMC.usedBytes();freeBytes=total>=used?total-used:0;}}{Guard g(stateMutex);state.uptime=now/1000;state.sdFree=freeBytes;}}
 if(now-uiTick>=200){uiTick=now;{Guard g(stateMutex);state.ota=Ota::status();}Ui::update(snapshot());if(videoPixels&&xSemaphoreTake(frameMutex,0)==pdTRUE){if(videoDirty){Ui::frame(videoPixels);videoDirty=false;}xSemaphoreGive(frameMutex);}}
 uint32_t idle=lv_disp_get_inactive_time(nullptr);Settings s=snapshot().settings;
 static bool dark=false;bool shouldDark=idle>(uint32_t)s.timeoutSec*1000&&!snapshot().talking&&!Ota::busy();
 if(shouldDark!=dark){dark=shouldDark;Board::brightness(dark?5:s.brightness);}
 lv_timer_handler();delay(5);
 static uint32_t healthTick=0;if(now-healthTick>=10000){healthTick=now;
  Serial.printf("[HEALTH] uptime=%lu heap=%u PSRAM=%u loop_stack_free_min=%u\n",
   (unsigned long)(now/1000),ESP.getFreeHeap(),ESP.getFreePsram(),(unsigned)uxTaskGetStackHighWaterMark(nullptr));
 }
}

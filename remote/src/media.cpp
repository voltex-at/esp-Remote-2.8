#include "runtime.h"
#include "ota.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TJpg_Decoder.h>
#include <esp_heap_caps.h>
#include <time.h>
namespace Media {
static uint8_t* jpg=nullptr;static uint16_t* decoded=nullptr;static uint16_t imageW=320,imageH=240;
static bool playPending=false;static uint32_t playBytes=0,playStamp=0;
static File clip,playFile;static bool playing=false;static uint32_t clipStart=0,playStart=0,nextPlay=0,lastFrame=0,lastStatus=0,frames=0;
static bool jpgBlock(int16_t x,int16_t y,uint16_t w,uint16_t h,uint16_t* src){
 for(int j=0;j<h;j++)for(int i=0;i<w;i++){
 int dx=(x+i)*224/imageW,dy=(y+j)*168/imageH;
 if(dx<224&&dy<168)decoded[dy*224+dx]=src[j*w+i];
 }return true;
}
static bool render(const uint8_t*data,size_t len){
 if(len<4||TJpgDec.getJpgSize(&imageW,&imageH,data,len)!=JDR_OK||imageW>640||imageH>480)return false;
 if(TJpgDec.drawJpg(0,0,data,len)!=JDR_OK)return false;
 {Guard g(frameMutex);memcpy(videoPixels,decoded,224*168*2);videoDirty=true;}return true;
}
static bool httpBegin(HTTPClient&http,WiFiClient&client,const Settings&s,const char*path){
 http.setConnectTimeout(700);http.setTimeout(900);if(!http.begin(client,String("http://")+s.cameraHost+path))return false;http.addHeader("X-Pair-Token",s.pairToken);return true;
}
static int fetch(const Settings&s){
 WiFiClient client;HTTPClient http;if(!httpBegin(http,client,s,"/capture"))return 0;
 int code=http.GET(),len=http.getSize();int got=0;
 if(code==200&&len>0&&(size_t)len<=MAX_JPEG_BYTES){got=http.getStreamPtr()->readBytes(jpg,len);if(got!=len)got=0;}http.end();return got;
}
static void control(const Settings&s,const char*var,int value){
 WiFiClient c;HTTPClient h;String p=String("/control?var=")+var+"&val="+value;
 if(httpBegin(h,c,s,p.c_str())){int code=h.GET();message(code==200?"Ustawienie kamery zapisane":"Kamera odrzucila ustawienie");h.end();}
}
static void status(const Settings&s){
 WiFiClient c;HTTPClient h;bool online=false;StaticJsonDocument<1536>d;
 if(httpBegin(h,c,s,"/status")){if(h.GET()==200){String body=h.getString();online=!deserializeJson(d,body)&&d["protocol"]==1;}h.end();}
 bool disconnected=false;{Guard g(stateMutex);disconnected=state.camera&&!online;state.camera=online;
 if(online){state.audioCap=d["cap"]["audio"]|false;state.motionCap=d["cap"]["motion"]|false;state.night=d["night"]|false;state.motionEnabled=d["motion_enabled"]|false;state.noiseEnabled=d["noise_enabled"]|false;}
 else{state.audioCap=state.motionCap=false;state.listening=state.talking=false;}}
 if(disconnected)alert("Utracono polaczenie z kamera");
 if(online){static uint32_t lastMotion=0,lastNoise=0;uint32_t motion=d["motion_seq"]|0,noise=d["noise_seq"]|0;if(motion!=lastMotion){if(lastMotion)alert("Kamera: wykryto ruch");lastMotion=motion;}if(noise!=lastNoise){if(lastNoise)alert("Kamera: glosny dzwiek");lastNoise=noise;}}
}
static void stopClip(){if(clip){Guard g(sdMutex);clip.flush();clip.close();}{Guard g(stateMutex);state.recording=false;state.clipMs=0;}message("Nagrywanie zakonczone");}
static void startClip(){
 Guard g(sdMutex);if(!Storage::available()){message("Nagrywanie wymaga SD i wolnego miejsca");return;}
 String path=Storage::newPath("/babyphone",".rvc");clip=SD_MMC.open(path,FILE_WRITE);if(!clip){message("Nie mozna utworzyc nagrania");return;}
 clip.write((const uint8_t*)"RVC1",4);clipStart=millis();frames=0;Guard st(stateMutex);state.recording=true;state.clipStarted=clipStart;strlcpy(state.clipPath,path.c_str(),sizeof state.clipPath);state.listening=state.audioCap&&Board::audioReady();
}
static bool saveFrame(size_t len){Guard g(sdMutex);if(!Storage::available())return false;uint32_t elapsed=millis()-clipStart,n=len;
 if(clip.write((uint8_t*)&elapsed,4)!=4||clip.write((uint8_t*)&n,4)!=4||clip.write(jpg,len)!=len)return false;
 if(++frames%20==0)clip.flush();return true;
}
static void snapshotPhoto(size_t len){Guard g(sdMutex);if(!Storage::available()){message("Zdjecie wymaga karty SD");return;}
 File f=SD_MMC.open(Storage::newPath("/babyphone",".jpg"),FILE_WRITE);bool ok=f&&f.write(jpg,len)==len;f.close();message(ok?"Zdjecie zapisane":"Blad zapisu zdjecia");}
static void stopPlayback(){Guard g(sdMutex);if(playFile)playFile.close();playing=false;playPending=false;Guard st(stateMutex);state.playing=false;}
static void openPlayback(const char*path){
 stopPlayback();Guard g(sdMutex);if(strncmp(path,"/babyphone/",11)||strstr(path,".."))return;
 playFile=SD_MMC.open(path);if(!playFile){message("Nie mozna otworzyc nagrania");return;}
 String p=path;if(p.endsWith(".jpg")){size_t n=playFile.size();if(n<=MAX_JPEG_BYTES&&playFile.read(jpg,n)==n)render(jpg,n);playFile.close();playing=true;nextPlay=UINT32_MAX;}
 else{char magic[4];if(playFile.read((uint8_t*)magic,4)!=4||memcmp(magic,"RVC1",4)){playFile.close();return;}playing=true;playStart=millis();nextPlay=0;}
 Guard st(stateMutex);state.playing=playing;
}
static void playbackStep(){
 if(!playFile)return;
 if(!playPending){
 uint32_t ts=0,n=0;int read=0;{Guard g(sdMutex);if(playFile.available()>=8){playFile.read((uint8_t*)&ts,4);playFile.read((uint8_t*)&n,4);if(n<=MAX_JPEG_BYTES&&n<=(uint32_t)playFile.available())read=playFile.read(jpg,n);}}
 if(!n||read!=(int)n){stopPlayback();return;}playBytes=n;playStamp=ts;playPending=true;
 }
 if(millis()-playStart>=playStamp){render(jpg,playBytes);playPending=false;}
}
void task(void*){
 jpg=(uint8_t*)ps_malloc(MAX_JPEG_BYTES);decoded=(uint16_t*)ps_malloc(224*168*2);
 if(!jpg||!decoded){message("Brak PSRAM dla obrazu kamery");vTaskDelete(nullptr);return;}
 TJpgDec.setJpgScale(1);TJpgDec.setSwapBytes(false);TJpgDec.setCallback(jpgBlock);
 for(;;){
 if(Ota::installing()){vTaskDelay(pdMS_TO_TICKS(50));continue;}
 AppState s=snapshot();Command cmd;
 while(xQueueReceive(mediaCommands,&cmd,0)==pdTRUE){switch(cmd.action){
 case Action::Snapshot:if(s.camera){int n=fetch(s.settings);if(n>0)snapshotPhoto(n);else message("Nie pobrano zdjecia");}else message("Kamera offline");break;
 case Action::Record:if(clip)stopClip();else if(s.camera&&!playing)startClip();else message("Kamera offline lub trwa odtwarzanie");break;
 case Action::CameraControl:if(s.camera)control(s.settings,cmd.text,cmd.value);else message("Kamera offline");lastStatus=0;break;
 case Action::Listen:{Guard g(stateMutex);state.listening=cmd.value&&state.audioCap&&Board::audioReady();if(!state.audioCap)strlcpy(state.message,"Ta kamera nie posiada audio",sizeof state.message);}break;
 case Action::Talk:{Guard g(stateMutex);state.talking=cmd.value&&state.audioCap&&Board::audioReady();if(!state.audioCap)strlcpy(state.message,"Ta kamera nie posiada audio",sizeof state.message);}break;
 case Action::LoadFiles:Storage::files();break;
 case Action::Play:if(cmd.value&& !clip)openPlayback(cmd.text);else stopPlayback();break;
 default:break;
 }}
 s=snapshot();Board::audioSession(s.settings.cameraHost,s.settings.pairToken,s.listening,s.talking);
 if(playing)playbackStep();
 if(!s.wifi){if(clip)stopClip();{Guard g(stateMutex);state.camera=false;state.listening=state.talking=false;}vTaskDelay(pdMS_TO_TICKS(100));continue;}
 if(millis()-lastStatus>=2500){lastStatus=millis();status(s.settings);}
 if(!playing&&millis()-lastFrame>=125){lastFrame=millis();int n=fetch(s.settings);
 if(n&&render(jpg,n)){Guard g(stateMutex);state.frames++;}
 if(clip){if(!n){stopClip();alert("Nagranie przerwane: brak obrazu");}else if(!saveFrame(n)){stopClip();alert("Nagranie przerwane: blad SD");}else if(millis()-clipStart>=MAX_CLIP_MS)stopClip();else{Guard g(stateMutex);state.clipMs=millis()-clipStart;}}
 }
 vTaskDelay(pdMS_TO_TICKS(10));
 }
}
}

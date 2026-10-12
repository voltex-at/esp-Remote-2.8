#include "runtime.h"
#include <bsec2.h>
#include <SensirionI2cScd4x.h>
#include <time.h>
namespace Sensors {
static Bsec2 bsec;
static SensirionI2cScd4x scd;
static bool scanning=true,bme=false,scdRunning=false,scdWaiting=false;
static uint8_t addr=8,bmeAddress=0,errors=0;
static uint32_t lastScan=0,nextScan=0,scdStart=0,scdRead=0,lastHistory=0,lastStateSave=0,retryBme=0;
static Preferences prefs;
void scan(){Guard g(stateMutex);state.scanRunning=true;}
static bool probe(uint8_t a){Wire.beginTransmission(a);return Wire.endTransmission()==0;}
static void callback(const bme68xData,const bsecOutputs out,const Bsec2 bs){
 Guard g(stateMutex);state.bsecStatus=bs.status;
 if(!out.nOutputs)return;
 for(uint8_t i=0;i<out.nOutputs;i++){const auto&o=out.output[i];switch(o.sensor_id){
 case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:state.temperature=o.signal;break;
 case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:state.humidity=o.signal;break;
 case BSEC_OUTPUT_RAW_PRESSURE:state.pressure=o.signal/100;break;
 case BSEC_OUTPUT_IAQ:state.iaq=o.signal;state.iaqAccuracy=o.accuracy;break;
 case BSEC_OUTPUT_CO2_EQUIVALENT:state.eco2=o.signal;break;
 case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:state.voc=o.signal;break;
 default:break;
 }}state.bmeMs=millis();state.bmeFresh=true;
}
static bool startBme(uint8_t a){
 if(!bsec.begin(a,Wire))return false;
 bsecSensor list[]={BSEC_OUTPUT_IAQ,BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,BSEC_OUTPUT_RAW_PRESSURE,BSEC_OUTPUT_CO2_EQUIVALENT,BSEC_OUTPUT_BREATH_VOC_EQUIVALENT};
 if(!bsec.updateSubscription(list,sizeof(list)/sizeof(list[0]),BSEC_SAMPLE_RATE_LP))return false;
 char key[8];snprintf(key,sizeof key,"bsec%02x",a);uint8_t blob[BSEC_MAX_STATE_BLOB_SIZE];
 if(prefs.getBytesLength(key)==sizeof blob){prefs.getBytes(key,blob,sizeof blob);if(!bsec.setState(blob)){prefs.remove(key);return false;}}
 bsec.attachCallback(callback);return true;
}
static void invalidateBme(){Guard g(stateMutex);state.bmeOnline=false;state.bmeFresh=false;state.temperature=state.humidity=state.iaq=state.pressure=state.eco2=state.voc=NAN;state.iaqAccuracy=0;}
static void invalidateScd(){Guard g(stateMutex);state.scdOnline=false;state.scdFresh=false;state.co2=state.scdTemperature=state.scdHumidity=NAN;}
static void recover(){
 Wire.end();pinMode(Pins::sda,INPUT_PULLUP);pinMode(Pins::scl,OUTPUT_OPEN_DRAIN);digitalWrite(Pins::scl,HIGH);
 for(int i=0;i<9&&digitalRead(Pins::sda)==LOW;i++){digitalWrite(Pins::scl,LOW);delayMicroseconds(5);digitalWrite(Pins::scl,HIGH);delayMicroseconds(5);}
 pinMode(Pins::sda,OUTPUT_OPEN_DRAIN);digitalWrite(Pins::sda,LOW);delayMicroseconds(5);digitalWrite(Pins::scl,HIGH);delayMicroseconds(5);digitalWrite(Pins::sda,HIGH);
 Wire.begin(Pins::sda,Pins::scl,100000);Wire.setTimeOut(20);
}
void task(void*){
 AppState*local=allocateStateCopy();if(!local){message("Brak PSRAM dla sensorow");vTaskDelete(nullptr);return;}AppState&s=*local;
 prefs.begin("bsec-state",false);scd.begin(Wire,0x62);
 {Guard g(stateMutex);state.scanRunning=true;for(auto&h:state.history)for(float&v:h)v=NAN;}
 for(;;){
 uint32_t now=millis();copyState(s);
 if(s.scanRunning&&!scanning){scanning=true;addr=8;Guard g(stateMutex);memset(state.found,0,sizeof state.found);}
 if(!scanning&&now-lastScan>=10000){scanning=true;addr=8;Guard g(stateMutex);state.scanRunning=true;memset(state.found,0,sizeof state.found);}
 if(scanning&&now-nextScan>=10){nextScan=now;bool seen;{Guard g(i2cMutex);if(digitalRead(Pins::sda)==LOW||digitalRead(Pins::scl)==LOW)recover();seen=probe(addr);}{Guard g(stateMutex);state.found[addr]=seen;}
 if(++addr>=120){scanning=false;lastScan=now;Guard g(stateMutex);state.scanRunning=false;}}
 copyState(s);uint8_t want=s.found[0x76]?0x76:s.found[0x77]?0x77:0;
 // Only change detection at a completed scan, avoiding false removal mid-scan.
 if(!scanning){
 if(bme&&!want){bme=false;invalidateBme();message("BME688 odlaczony");}
 if(!bme&&want&&now-retryBme>=3000){retryBme=now;bool ok;{Guard g(i2cMutex);ok=startBme(want);}bme=ok;bmeAddress=want;{Guard g(stateMutex);state.bmeOnline=ok;state.sensorAddress=want;state.bsecStatus=bsec.status;state.bmeStatus=bsec.sensor.status;}if(!ok)message("BME688: blad inicjalizacji / BSEC2");}
 if(!s.found[0x62]){scdRunning=scdWaiting=false;invalidateScd();}
 if(s.found[0x62]&&!scdRunning&&!scdWaiting){
 // Stop command's 500ms settling period runs outside the shared bus lock.
 {Guard g(i2cMutex);Wire.beginTransmission(0x62);Wire.write(0x3f);Wire.write(0x86);if(Wire.endTransmission()==0){scdWaiting=true;scdStart=now;}}
 }
 }
 if(scdWaiting&&now-scdStart>=500){int16_t e;{Guard g(i2cMutex);e=scd.startPeriodicMeasurement();}scdWaiting=false;scdRunning=e==0;scdRead=now;Guard g(stateMutex);state.scdOnline=scdRunning;}
 if(bme){bool ok;{Guard g(i2cMutex);ok=bsec.run();}{Guard g(stateMutex);state.bsecStatus=bsec.status;state.bmeStatus=bsec.sensor.status;}
 if(!ok&&(bsec.status<0||bsec.sensor.status<0)){bme=false;invalidateBme();}
 if(now-lastStateSave>=6UL*60*60*1000&&s.iaqAccuracy>=2){uint8_t blob[BSEC_MAX_STATE_BLOB_SIZE];if(bsec.getState(blob)){char key[8];snprintf(key,sizeof key,"bsec%02x",bmeAddress);prefs.putBytes(key,blob,sizeof blob);lastStateSave=now;}}
 // Save first accurate state once, then six-hour cadence; avoids flash wear.
 if(lastStateSave==0&&s.iaqAccuracy>=2){uint8_t blob[BSEC_MAX_STATE_BLOB_SIZE];if(bsec.getState(blob)){char key[8];snprintf(key,sizeof key,"bsec%02x",bmeAddress);prefs.putBytes(key,blob,sizeof blob);lastStateSave=now;}}
 }
 if(scdRunning&&now-scdRead>=1000){scdRead=now;bool ready=false;uint16_t co2=0;float t,rh;int16_t e;
 {Guard g(i2cMutex);e=scd.getDataReadyStatus(ready);if(!e&&ready)e=scd.readMeasurement(co2,t,rh);}
 if(e){if(++errors>=3){scdRunning=false;errors=0;invalidateScd();}}
 else {errors=0;if(ready&&co2){Guard g(stateMutex);state.co2=co2;state.scdTemperature=t;state.scdHumidity=rh;state.scdMs=now;state.scdFresh=true;}}
 }
 {Guard g(stateMutex);state.bmeFresh=bme&&state.bmeMs&&now-state.bmeMs<15000;state.scdFresh=scdRunning&&state.scdMs&&now-state.scdMs<20000;}
 copyState(s);if(now-lastHistory>=(uint32_t)s.settings.logIntervalSec*1000){lastHistory=now;
 {Guard g(stateMutex);for(int m=0;m<METRIC_COUNT;m++){for(int i=1;i<HISTORY_COUNT;i++)state.history[m][i-1]=state.history[m][i];state.history[m][HISTORY_COUNT-1]=metricValid(state,m)?metricValue(state,m):NAN;}if(state.historyCount<HISTORY_COUNT)state.historyCount++;state.sampleMs=now;}
 copyState(s);Storage::logSample(s);
 static uint8_t alarmBits=0;uint8_t bits=0;if(metricValid(s,0)){float temp=metricValue(s,0),rh=metricValue(s,1);if(temp<s.settings.tempMin||temp>s.settings.tempMax)bits|=1;if(rh<s.settings.humidityMin||rh>s.settings.humidityMax)bits|=2;}if(s.bmeFresh&&s.iaqAccuracy>=2&&s.iaq>s.settings.iaqMax)bits|=8;if(s.scdFresh&&s.co2>s.settings.co2Max)bits|=4;
 const char* texts[]={"Temperatura poza zakresem","Wilgotnosc poza zakresem","Wysokie CO2 (SCD41)","Wysoki IAQ (BME688)"};for(int i=0;i<4;i++)if((bits&(1<<i))&&!(alarmBits&(1<<i)))alert(texts[i]);alarmBits=bits;
 }
 vTaskDelay(pdMS_TO_TICKS(10));
 }
}
}

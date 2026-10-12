#include "runtime.h"
#include "sensor_hub.h"
#include <NimBLEDevice.h>
#include <freertos/event_groups.h>
#include <esp_heap_caps.h>
#include <atomic>
namespace SensorHub {
using namespace SensorRegistry;
static QueueHandle_t queue;static EventGroupHandle_t events;
static constexpr EventBits_t Pause=1,Ack=2;
static Preferences prefs;static bool initialized=false;static std::atomic<bool> discovering{false};
static uint32_t scanDeadline=0,lastEss=0,lastHistory=0;static uint8_t essSlot=0;
static bool paused(){return xEventGroupGetBits(events)&Pause;}
static Registry configs(){Registry r;Guard g(stateMutex);r.count=state.sensorCount;for(int i=0;i<r.count;i++)r.devices[i]=state.sensors[i].config;return r;}
static bool save(const Registry&r){if(prefs.getUInt("schema",0)!=1&&prefs.putUInt("schema",1)!=sizeof(uint32_t))return false;return prefs.putBytes("devices",&r,sizeof r)==sizeof r;}
static void error(BleError e){Guard g(stateMutex);if(state.bleError==e&&state.bleState==BleState::Error)return;state.bleError=e;state.bleState=BleState::Error;state.bleGeneration++;if(e==BleError::Save)strlcpy(state.message,state.settings.language==1?"Sensor konnte nicht gespeichert werden.":state.settings.language==2?"Sensor could not be saved.":"Nie udalo sie zapisac sensora.",sizeof state.message);}
static void applyReading(Device&d,const Reading&r,uint32_t now){for(int m=0;m<Measurements;m++)if(r.mask&(1u<<m)){d.values[m]=r.values[m];d.updated[m]=now;}}
class Advertisements:public NimBLEAdvertisedDeviceCallbacks {
 void onResult(NimBLEAdvertisedDevice*a) override {
  if(paused())return;BleSensor found;std::string address=a->getAddress().toString();strlcpy(found.config.address,address.c_str(),sizeof found.config.address);found.config.addressType=a->getAddress().getType();found.rssi=a->getRSSI();
  std::string payload=a->getServiceData(NimBLEUUID(uint16_t(0xfcd2)));
  if(!payload.empty()){
   found.config.source=Source::Bthome;auto result=bthome((const uint8_t*)payload.data(),payload.size(),found.reading);found.encrypted=result==Decode::Encrypted;
   if(result!=Decode::Valid&&!found.encrypted)return;found.config.capabilities=found.reading.mask;
  }else if(a->isAdvertisingService(NimBLEUUID(uint16_t(0x181a))))found.config.source=Source::Ess;
  else return;
  std::string name=a->haveName()?a->getName():"";strlcpy(found.config.name,name.empty()?(found.config.source==Source::Bthome?"BTHome":"BLE ESS"):name.c_str(),sizeof found.config.name);
  uint32_t now=millis();for(int m=0;m<Measurements;m++)if(found.reading.mask&(1u<<m))found.updated[m]=now;Guard g(stateMutex);
  for(int i=0;i<state.sensorCount;i++)if(same(state.sensors[i].config,found.config)){auto&d=state.sensors[i];d.rssi=found.rssi;applyReading(d,found.reading,now);}
  if(!discovering)return;int slot=-1;for(int i=0;i<state.bleCount;i++)if(same(state.bleDevices[i].config,found.config)){slot=i;break;}
  if(slot<0){if(state.bleCount>=12)return;slot=state.bleCount++;state.bleGeneration++;}
  // Keep first identity/name stable while rows are under a finger. Telemetry
  // updates do not rebuild the list every advertising packet.
  auto&d=state.bleDevices[slot];if(!d.config.address[0])d=found;else{d.rssi=found.rssi;uint8_t capabilities=d.config.capabilities|found.config.capabilities;for(int m=0;m<Measurements;m++)if(found.reading.mask&(1u<<m)){d.reading.values[m]=found.reading.values[m];d.updated[m]=now;}d.reading.mask|=found.reading.mask;if(d.config.capabilities!=capabilities||d.encrypted!=found.encrypted){d.config.capabilities=capabilities;d.encrypted=found.encrypted;state.bleGeneration++;}}

 }
};
static Advertisements advertisements;
static void complete(NimBLEScanResults){/* The owner task publishes scan state. */}
static bool init(){if(initialized)return true;if(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<28000||heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<60000){error(BleError::Memory);return false;}
 NimBLEDevice::init("");initialized=NimBLEDevice::getInitialized();if(!initialized){error(BleError::Start);return false;}
 NimBLEDevice::setMTU(64);auto*s=NimBLEDevice::getScan();s->setAdvertisedDeviceCallbacks(&advertisements,true);s->setMaxResults(0);s->setActiveScan(true);s->setInterval(100);s->setWindow(30);return true;
}
static void stop(){if(initialized){NimBLEDevice::getScan()->stop();NimBLEDevice::deinit(true);initialized=false;}discovering=false;}
static bool readEss(const Config&c,Reading&r,int16_t&rssi){if(!init()||paused())return false;NimBLEDevice::getScan()->stop();auto*client=NimBLEDevice::createClient();if(!client)return false;client->setConnectTimeout(3);
 bool ok=client->connect(NimBLEAddress(std::string(c.address),c.addressType));if(ok){rssi=client->getRssi();const uint16_t ids[]={0x2a6e,0x2a6f,0x2a6d,0x2a19};
  for(auto id:ids){if(paused())break;auto*s=client->getService(NimBLEUUID(uint16_t(id==0x2a19?0x180f:0x181a)));if(!s)continue;auto*v=s->getCharacteristic(NimBLEUUID(id));if(!v||!v->canRead())continue;auto bytes=v->readValue();ess(id,bytes.data(),bytes.size(),r);}
 }if(client->isConnected())client->disconnect();NimBLEDevice::deleteClient(client);return ok&&r.mask&&!paused();
}
static void addDevice(const Command&cmd){Config c;Reading reading;uint32_t stamps[Measurements]={};int16_t rssi=-127;
 if(cmd.value==0||cmd.value==1){Guard g(stateMutex);bool bme=cmd.value==0;if(!(bme?state.bmeOnline:state.scdOnline)){state.bleError=BleError::Read;state.bleState=BleState::Error;state.bleGeneration++;return;}c.source=bme?Source::Bme688:Source::Scd41;c.capabilities=bme?0x3b:0x07;strlcpy(c.name,bme?"BME688 · POGO":"SCD41 · POGO",sizeof c.name);strlcpy(c.address,bme?"I2C:BME":"I2C:SCD",sizeof c.address);
 }else{
  bool found=false,encrypted=false;{Guard g(stateMutex);for(int i=0;i<state.bleCount;i++)if(!strcmp(cmd.text,state.bleDevices[i].config.address)){auto&d=state.bleDevices[i];c=d.config;reading=d.reading;memcpy(stamps,d.updated,sizeof stamps);rssi=d.rssi;encrypted=d.encrypted;found=true;break;}}
  if(!found){error(BleError::Read);return;}if(encrypted){error(BleError::Encrypted);return;}
  if(c.source==Source::Ess){{Guard g(stateMutex);state.bleState=BleState::Reading;state.bleGeneration++;}if(!readEss(c,reading,rssi)){error(BleError::Read);return;}c.capabilities=reading.mask;for(int m=0;m<Measurements;m++)if(reading.mask&(1u<<m))stamps[m]=millis();}
 }
 if(!c.capabilities){error(BleError::Unsupported);return;}Registry r=configs();int index=add(r,c);if(index<0){error(BleError::Full);return;}if(!save(r)){error(BleError::Save);return;}
 {Guard g(stateMutex);if(index>=state.sensorCount){state.sensors[index]=Device{};state.sensors[index].config=c;state.sensorCount=r.count;}auto&d=state.sensors[index];d.rssi=rssi;applyReading(d,reading,millis());for(int m=0;m<Measurements;m++)if(reading.mask&(1u<<m))d.updated[m]=stamps[m];state.lastAdded=index;state.sensorAddedGeneration++;state.sensorGeneration++;state.bleError=BleError::None;state.bleState=BleState::Done;state.bleGeneration++;}
}
static void modify(const Command&c){Registry r=configs();int slot=c.action==Action::SensorSelect?c.value/8:c.value,m=c.value%8;if(slot<0||slot>=r.count||strcmp(c.text,r.devices[slot].address))return;
 if(c.action==Action::SensorSelect){if(!supported(r.devices[slot],m))return;r.devices[slot].selected=m;}else{for(int i=slot+1;i<r.count;i++)r.devices[i-1]=r.devices[i];r.devices[--r.count]=Config{};}
 if(!save(r)){error(BleError::Save);return;}Guard g(stateMutex);
 if(c.action==Action::SensorSelect){state.sensors[slot].config.selected=m;state.sensors[slot].historyCount=0;for(float&v:state.sensors[slot].history)v=NAN;}
 else{for(int i=slot+1;i<state.sensorCount;i++)state.sensors[i-1]=state.sensors[i];state.sensors[state.sensorCount-1]=Device{};state.sensorCount=r.count;}
 state.sensorGeneration++;
}
bool begin(){queue=xQueueCreate(8,sizeof(Command));events=xEventGroupCreate();if(!queue||!events)return false;prefs.begin("sensor-hub",false);Registry r;
 if(prefs.getUInt("schema",0)==1&&prefs.getBytesLength("devices")==sizeof r){prefs.getBytes("devices",&r,sizeof r);if(valid(r)){Guard g(stateMutex);state.sensorCount=r.count;for(int i=0;i<r.count;i++)state.sensors[i].config=r.devices[i];state.sensorGeneration++;}}
 {Guard g(stateMutex);for(auto&d:state.sensors)for(float&v:d.history)v=NAN;}return true;
}
void command(const Command&c){if(xQueueSend(queue,&c,0)!=pdTRUE)error(BleError::Memory);}
bool pause(uint32_t timeoutMs){if(!events)return true;xEventGroupSetBits(events,Pause);return xEventGroupWaitBits(events,Ack,pdFALSE,pdTRUE,pdMS_TO_TICKS(timeoutMs))&Ack;}
void resume(){if(events)xEventGroupClearBits(events,Pause);}
bool reset(){bool ok=pause(12000);if(ok){ok=prefs.clear();xQueueReset(queue);}resume();return ok;}
void task(void*){
 for(;;){uint32_t now=millis();if(paused()){stop();xEventGroupSetBits(events,Ack);while(paused())vTaskDelay(pdMS_TO_TICKS(20));xEventGroupClearBits(events,Ack);lastEss=0;}
  Command cmd;while(!paused()&&xQueueReceive(queue,&cmd,0)==pdTRUE){
   if(cmd.action==Action::BleScan){if(init()){NimBLEDevice::getScan()->stop();{Guard g(stateMutex);state.bleCount=0;for(auto&d:state.bleDevices)d=BleSensor{};state.bleState=BleState::Scanning;state.bleError=BleError::None;state.bleGeneration++;}NimBLEDevice::getScan()->setActiveScan(true);discovering=true;scanDeadline=millis()+15000;if(!NimBLEDevice::getScan()->start(15,complete,false)){discovering=false;error(BleError::Start);}}}
   else if(cmd.action==Action::SensorAdd)addDevice(cmd);else modify(cmd);
  }
  if(discovering&&(int32_t)(now-scanDeadline)>=0){discovering=false;if(initialized)NimBLEDevice::getScan()->setActiveScan(false);Guard g(stateMutex);if(state.bleState==BleState::Scanning)state.bleState=BleState::Done;state.bleGeneration++;}
  Registry r=configs();bool bthomeNeeded=false;for(int i=0;i<r.count;i++)bthomeNeeded|=r.devices[i].source==Source::Bthome;
  if(!discovering&&now-lastEss>=15000){lastEss=now;for(int j=0;j<r.count;j++){int i=(essSlot+j)%r.count;if(r.devices[i].source!=Source::Ess)continue;essSlot=(i+1)%r.count;Reading reading;int16_t rssi=-127;if(readEss(r.devices[i],reading,rssi)){Guard g(stateMutex);if(i<state.sensorCount&&same(state.sensors[i].config,r.devices[i])){applyReading(state.sensors[i],reading,millis());state.sensors[i].rssi=rssi;}}break;}}
  if(bthomeNeeded&&!paused()&&init()&&!NimBLEDevice::getScan()->isScanning())NimBLEDevice::getScan()->start(0,complete,false);
  if(!bthomeNeeded&&!discovering&&initialized)stop();
  now=millis();{Guard g(stateMutex);state.nowMs=now;for(int i=0;i<state.sensorCount;i++){auto&d=state.sensors[i];bool bme=d.config.source==Source::Bme688,scd=d.config.source==Source::Scd41;if(bme||scd){bool fresh=bme?state.bmeFresh:state.scdFresh;uint32_t stamp=bme?state.bmeMs:state.scdMs;if(fresh){Reading v;v.mask=d.config.capabilities;v.values[0]=bme?state.temperature:state.scdTemperature;v.values[1]=bme?state.humidity:state.scdHumidity;v.values[2]=state.co2;v.values[3]=state.iaq;v.values[4]=state.pressure;v.values[5]=state.eco2;applyReading(d,v,stamp);}else for(auto&t:d.updated)t=0;}
    if(now-lastHistory>=(uint32_t)state.settings.logIntervalSec*1000&&d.config.selected!=None){for(int j=1;j<60;j++)d.history[j-1]=d.history[j];d.history[59]=fresh(d,d.config.selected,now)?d.values[d.config.selected]:NAN;if(d.historyCount<60)d.historyCount++;}
   }if(now-lastHistory>=(uint32_t)state.settings.logIntervalSec*1000)lastHistory=now;
  }vTaskDelay(pdMS_TO_TICKS(100));
 }
}
}

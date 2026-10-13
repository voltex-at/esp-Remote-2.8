#pragma once
#include "sensor_registry.h"

// One WS2812B illuminates both case light pipes. Persist an identity, not an
// index; removal or reordering must never transfer an alarm to another sensor.
namespace SignalLed {
enum class Mode:uint8_t { Fixed,Co2,Off };
struct Config {
 Mode mode=Mode::Fixed;
 uint8_t brightness=30,red=0,green=255,blue=0,sensorSet=0;
 SensorRegistry::Source source=SensorRegistry::Source::Scd41;
 char address[18]={};
};
static_assert(sizeof(Config)==25,"LED NVS schema1 layout");
constexpr uint32_t Green=0x00ff00,Orange=0xff7800,Red=0xff0000,NoData=0x0088ff;
inline bool valid(const Config&c){
 if(unsigned(c.mode)>unsigned(Mode::Off)||c.brightness>100||c.sensorSet>1||c.address[17])return false;
 if(!c.sensorSet)return true;
 SensorRegistry::Config identity;identity.source=c.source;identity.capabilities=1u<<2;
 memcpy(identity.address,c.address,sizeof identity.address);return SensorRegistry::valid(identity);
}
template<class Store> Config load(Store&store){
 Config c;if(store.getUInt("led-schema",0)!=1||store.getBytesLength("led-config")!=sizeof c||store.getBytes("led-config",&c,sizeof c)!=sizeof c||!valid(c))return Config{};return c;
}
template<class Store> bool save(Store&store,const Config&c){
 return valid(c)&&store.putBytes("led-config",&c,sizeof c)==sizeof c&&store.putUInt("led-schema",1)==sizeof(uint32_t);
}
inline bool same(const Config&a,const Config&b){return a.mode==b.mode&&a.brightness==b.brightness&&a.red==b.red&&a.green==b.green&&a.blue==b.blue&&a.sensorSet==b.sensorSet&&a.source==b.source&&!strcmp(a.address,b.address);}
inline uint32_t color(const Config&c){return uint32_t(c.red)<<16|uint32_t(c.green)<<8|c.blue;}
inline void color(Config&c,uint32_t rgb){c.red=rgb>>16;c.green=rgb>>8;c.blue=rgb;}
inline void select(Config&c,const SensorRegistry::Config&sensor){c.sensorSet=1;c.source=sensor.source;memcpy(c.address,sensor.address,sizeof c.address);}
inline bool choose(Config&c,const SensorRegistry::Device*devices,uint8_t count,int encoded,const char*address){
 if(encoded<0||!address||!devices)return false;int slot=encoded/4;
 if(slot>=count||slot>=SensorRegistry::Capacity)return false;const auto&sensor=devices[slot].config;
 if(encoded%4!=int(sensor.source)||strcmp(address,sensor.address)||!SensorRegistry::valid(sensor)||!SensorRegistry::supported(sensor,2))return false;
 Config next=c;select(next,sensor);next.mode=Mode::Co2;if(!valid(next))return false;c=next;return true;
}
inline bool parseColor(const char*text,uint32_t&rgb){
 if(!text||strlen(text)!=6)return false;uint32_t value=0;
 for(int i=0;i<6;i++){char c=text[i];unsigned digit;if(c>='0'&&c<='9')digit=c-'0';else if(c>='a'&&c<='f')digit=c-'a'+10;else if(c>='A'&&c<='F')digit=c-'A'+10;else return false;value=value*16+digit;}
 rgb=value;return true;
}
inline int sensorIndex(const Config&c,const SensorRegistry::Device*devices,uint8_t count){
 if(!c.sensorSet||!valid(c))return -1;
 for(int i=0;i<count&&i<SensorRegistry::Capacity;i++)if(devices[i].config.source==c.source&&!strcmp(devices[i].config.address,c.address)&&SensorRegistry::supported(devices[i].config,2))return i;return -1;
}
inline uint32_t scaled(uint32_t rgb,unsigned percent){if(percent>100)percent=100;uint32_t r=((rgb>>16)&255)*percent/100,g=((rgb>>8)&255)*percent/100,b=(rgb&255)*percent/100;return r<<16|g<<8|b;}
enum class Band:uint8_t { None,Green,Orange,Red };
// Escalate at exactly 1200/1500 ppm; 50 ppm return hysteresis prevents flicker.
// Missing/stale data immediately invalidates the band; it is never green.
class Monitor {
 Band band_=Band::None;Config previous_;bool haveConfig_=false;
public:
 Band band()const{return band_;}
 uint32_t sample(const Config&c,const SensorRegistry::Device*devices,uint8_t count,uint32_t now){
  if(!valid(c))return 0;
  if(!haveConfig_||c.mode!=previous_.mode||c.sensorSet!=previous_.sensorSet||c.source!=previous_.source||strcmp(c.address,previous_.address)){band_=Band::None;}
  previous_=c;haveConfig_=true;
  if(c.mode==Mode::Off)return 0;
  if(c.mode==Mode::Fixed)return scaled(color(c),c.brightness);
  int slot=sensorIndex(c,devices,count);
  if(slot<0||!SensorRegistry::fresh(devices[slot],2,now)||devices[slot].values[2]<0){band_=Band::None;return scaled((now/1000)%2?0:NoData,c.brightness);}
  float ppm=devices[slot].values[2];
  if(ppm>=1500)band_=Band::Red;
  else if(band_==Band::Red&&ppm>=1450){}
  else if(ppm>=1200)band_=Band::Orange;
  else if((band_==Band::Orange||band_==Band::Red)&&ppm>=1150)band_=Band::Orange;
  else band_=Band::Green;
  return scaled(band_==Band::Red?Red:band_==Band::Orange?Orange:Green,c.brightness);
 }
};
class Controller {
 Monitor monitor_;bool ready_=false;uint32_t readyAt_=0;
public:
 void begin(){ready_=false;monitor_=Monitor{};}
 void ready(uint32_t now){ready_=true;readyAt_=now;}
 uint32_t sample(const Config&c,const SensorRegistry::Device*devices,uint8_t count,uint32_t now){
  if(!ready_)return scaled(Red,20);
  if(now-readyAt_<1500)return scaled(Green,20);
  return monitor_.sample(c,devices,count,now);
 }
};
}

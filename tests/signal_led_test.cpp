#include "app_state.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
using namespace SignalLed;
extern "C" int __lsan_is_turned_off(){return 1;} // executor cannot inspect /proc; ASan/UBSan remain enabled
struct Store {
 uint32_t schema=0;size_t length=0;bool fail=false,partial=false;Config bytes;
 uint32_t getUInt(const char*k,uint32_t fallback){assert(!strcmp(k,"led-schema"));return schema?schema:fallback;}
 size_t getBytesLength(const char*k){assert(!strcmp(k,"led-config"));return length;}
 size_t getBytes(const char*k,void*p,size_t n){assert(!strcmp(k,"led-config"));memcpy(p,&bytes,n);return partial?n-1:n;}
 size_t putBytes(const char*k,const void*p,size_t n){assert(!strcmp(k,"led-config"));if(fail)return 0;memcpy(&bytes,p,n);length=n;return n;}
 size_t putUInt(const char*k,uint32_t s){assert(!strcmp(k,"led-schema"));if(fail)return 0;schema=s;return sizeof s;}
};
int main(){
 static_assert(sizeof(Settings)==264,"Legacy settings unchanged");
 Config c;assert(valid(c)&&c.mode==Mode::Fixed&&color(c)==Green&&c.brightness==30);
 uint32_t parsed=0;assert(parseColor("ABCdef",parsed)&&parsed==0xabcdef);assert(parseColor("000000",parsed)&&parsed==0);assert(parseColor("ffffff",parsed)&&parsed==0xffffff);
 for(const char*bad:{"","00000","0000000","+00000","-00000"," 00000","0x1234","#12345","12345g"}){parsed=123;assert(!parseColor(bad,parsed)&&parsed==123);}assert(!parseColor(nullptr,parsed));
 Store store;assert(same(load(store),c));color(c,0xabcdef);c.brightness=67;assert(save(store,c));assert(same(load(store),c));
 store.fail=true;assert(!save(store,c));store.fail=false;store.partial=true;assert(same(load(store),Config{}));store.partial=false;
 for(auto bad:{2u,9u}){store.schema=bad;assert(same(load(store),Config{}));}store.schema=1;store.length=24;assert(same(load(store),Config{}));store.length=25;
 for(auto bad:{101,255}){store.bytes.brightness=bad;assert(same(load(store),Config{}));}store.bytes=c;store.bytes.mode=Mode(99);assert(same(load(store),Config{}));store.bytes=c;store.bytes.sensorSet=2;assert(same(load(store),Config{}));store.bytes=c;store.bytes.address[17]='x';assert(same(load(store),Config{}));
 SensorRegistry::Device devices[4];auto&d=devices[0];d.config.source=SensorRegistry::Source::Scd41;strcpy(d.config.address,"I2C:SCD");strcpy(d.config.name,"SCD41");d.config.capabilities=0x07;d.config.selected=0;d.values[2]=700;d.updated[2]=1000;
 c=Config{};c.mode=Mode::Co2;c.brightness=100;select(c,d.config);assert(valid(c)&&sensorIndex(c,devices,1)==0);Monitor m;
 Config chosen;assert(choose(chosen,devices,1,int(d.config.source),d.config.address)&&chosen.mode==Mode::Co2&&sensorIndex(chosen,devices,1)==0);
 Config unchanged=chosen;for(int encoded:{-1,0,2,7,99})assert(!choose(chosen,devices,1,encoded,d.config.address)&&same(chosen,unchanged));assert(!choose(chosen,devices,1,int(d.config.source),"wrong")&&same(chosen,unchanged));d.config.capabilities=1;assert(!choose(chosen,devices,1,int(d.config.source),d.config.address));d.config.capabilities=7;
 auto read=[&](float ppm,uint32_t now=2000){d.values[2]=ppm;return m.sample(c,devices,1,now);};
 assert(read(700)==Green);assert(read(1199)==Green);assert(read(1200)==Orange);assert(read(1499)==Orange);assert(read(1500)==Red);assert(read(1800)==Red);
 assert(read(1499)==Red);assert(read(1450)==Red);assert(read(1449)==Orange);assert(read(1199)==Orange);assert(read(1150)==Orange);assert(read(1149)==Green);
 assert(read(1500)==Red);c.brightness=40;assert(read(1499)==scaled(Red,40));c.brightness=100;
 assert(read(NAN)==NoData&&m.band()==Band::None);assert(read(INFINITY)==NoData);assert(read(-1)==NoData);assert(read(1200)==Orange);assert(read(700,31000)!=Green&&m.band()==Band::None);
 d.updated[2]=0;assert(read(700)==NoData);d.updated[2]=1000;assert(m.sample(c,devices,0,2000)==NoData);assert(m.sample(c,devices,0,3000)==0);
 // Source freshness is evaluated per CO2 field, not another measurement.
 d.config.source=SensorRegistry::Source::Bthome;strcpy(d.config.address,"aa:bb:cc:dd:ee:01");select(c,d.config);d.updated[0]=181000;d.updated[2]=1000;assert(m.sample(c,devices,1,181000)!=Green);assert(sensorIndex(c,devices,1)==0);
 auto backup=d;devices[0]=SensorRegistry::Device{};devices[1]=backup;assert(sensorIndex(c,devices,2)==1);assert(sensorIndex(c,devices,1)==-1);devices[1].config.source=SensorRegistry::Source::Ess;assert(sensorIndex(c,devices,2)==-1);
 devices[0]=backup;d.updated[2]=0xfffffff0;d.values[2]=1500;assert(m.sample(c,devices,1,0x20)==Red); // millis rollover
 c.mode=Mode::Fixed;color(c,0xabcdef);c.brightness=0;assert(m.sample(c,devices,1,0x20)==0);c.brightness=100;assert(m.sample(c,devices,1,0x20)==0xabcdef);c.mode=Mode::Off;assert(m.sample(c,devices,1,0x20)==0);
 c.mode=Mode::Co2;c.brightness=30;assert(m.sample(c,devices,1,0x20)==scaled(Red,30));c.mode=Mode(99);assert(m.sample(c,devices,1,0x20)==0);
 Controller controller;c=Config{};c.mode=Mode::Off;assert(controller.sample(c,nullptr,0,100000)==scaled(Red,20));controller.ready(0xfffffff0);assert(controller.sample(c,nullptr,0,0x20)==scaled(Green,20));assert(controller.sample(c,nullptr,0,uint32_t(0xfffffff0+1500u))==0);controller.begin();assert(controller.sample(c,nullptr,0,10)==scaled(Red,20));
 c=Config{};c.sensorSet=1;c.source=SensorRegistry::Source::Bthome;strcpy(c.address,"not a MAC");assert(!valid(c));c.source=SensorRegistry::Source(99);assert(!valid(c));
 puts("LED: boot/ready, arbitrary RGB, brightness/off, exact thresholds, hysteresis, stale/missing/NaN, per-field TTL, identity/reorder, rollover, NVS upgrade/reboot/corruption and unchanged schema1 OK.");
}

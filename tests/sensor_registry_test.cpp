#include "sensor_registry.h"
#include <assert.h>
#include <stdio.h>
extern "C" int __lsan_is_turned_off(){return 1;}
using namespace SensorRegistry;
int main(){
 Reading r;uint8_t sample[]={0x40,0x00,0x21,0x01,97,0x02,0xc4,0x09,0x03,0xbf,0x13,0x04,0x13,0x8a,0x01,0x12,0xe2,0x04};
 assert(bthome(sample,sizeof sample,r)==Decode::Valid&&r.mask==0x57);assert(fabs(r.values[0]-25)<.001&&fabs(r.values[1]-50.55)<.001&&fabs(r.values[4]-1008.83)<.01&&r.values[2]==1250&&r.values[6]==97);
 uint8_t cold[]={0x40,0x02,0x0c,0xfe};assert(bthome(cold,sizeof cold,r)==Decode::Valid&&fabs(r.values[0]+5)<.001);
 // Every truncated object must reject the entire frame; no partial readings
 // become fresh after a malformed packet. Header-only is unsupported.
 for(size_t n=0;n<sizeof sample;n++){auto result=bthome(sample,n,r);bool boundary=n==3||n==5||n==8||n==11||n==15;if(boundary)assert(result==Decode::Valid||result==Decode::Unsupported);else assert(result==Decode::Malformed||result==Decode::Unsupported);}
 uint8_t repeated[]={0x40,2,0xc4,9,2,0x10,0x27};assert(bthome(repeated,sizeof repeated,r)==Decode::Valid&&r.values[0]==25);
 uint8_t encrypted[]={0x41,1,2,3};assert(bthome(encrypted,sizeof encrypted,r)==Decode::Encrypted);
 uint8_t newer[]={0x60,2,1,2};assert(bthome(newer,sizeof newer,r)==Decode::Unsupported);
 uint8_t unknown[]={0x40,2,0xc4,9,0xff,0};assert(bthome(unknown,sizeof unknown,r)==Decode::Valid&&r.mask==1);
 uint8_t volts[]={0x40,0x0c,0x02,0x0c,0x12,0xe2,4};assert(bthome(volts,sizeof volts,r)==Decode::Valid&&r.values[2]==1250);
 uint8_t humid[]={0x40,0x03,0xff,0xff};assert(bthome(humid,sizeof humid,r)==Decode::Malformed);
 uint8_t pressure[]={0x14,0x76,0x0f,0};r=Reading{};assert(ess(0x2a6d,pressure,4,r)&&fabs(r.values[4]-1013.268)<.01);assert(!ess(0x2a6d,pressure,3,r));assert(!ess(0x2a6d,pressure,5,r));assert(!ess(0x9999,pressure,4,r));
 uint8_t temp[]={0x0c,0xfe};assert(ess(0x2a6e,temp,2,r)&&fabs(r.values[0]+5)<.001);
 Registry reg;Config c;c.source=Source::Bthome;c.capabilities=3;strcpy(c.address,"aa:bb:cc:dd:ee:01");strcpy(c.name,"Living room");assert(valid(c));assert(add(reg,c)==0&&reg.count==1&&reg.devices[0].selected==None);assert(add(reg,c)==0&&reg.count==1);
 c.address[0]='z';assert(!valid(c));c.address[0]='a';
 c.selected=2;assert(!valid(c));c.selected=1;assert(valid(c));
 for(int i=2;i<=4;i++){c.address[16]='0'+i;assert(add(reg,c)==i-1);}c.address[16]='5';assert(add(reg,c)==-1&&reg.count==4&&valid(reg));
 Registry restored;memcpy(&restored,&reg,sizeof reg);assert(valid(restored)&&restored.count==4&&restored.devices[3].selected==1);restored.count=5;assert(!valid(restored));restored=reg;restored.devices[1]=restored.devices[0];assert(!valid(restored));
 Device d;d.config=c;d.values[0]=23;d.updated[0]=100;assert(fresh(d,0,1000)&&!fresh(d,1,1000)&&!fresh(d,0,180100));d.config.source=Source::Ess;assert(!fresh(d,0,30100));d.updated[0]=0;assert(!fresh(d,0,1000));d.updated[0]=0xfffffff0;assert(fresh(d,0,20));
 puts("Sensors: BTHome/ESS vectors, signed/scaled values, truncated and encrypted packets, persistence, duplicate/capacity checks and per-value freshness OK.");
}

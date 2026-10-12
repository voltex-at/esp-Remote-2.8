#pragma once
#include <stdint.h>
#include <math.h>
#include <algorithm>
namespace Battery {
// Voltage-only estimate requested for this board; this is not a charger-status pin.
enum class Power:uint8_t { Battery,Charging,External };
struct Reading {float voltage=NAN;int percent=-1;Power power=Power::Battery;};
inline int percentage(float v){
 if(!isfinite(v)||v<2.5f||v>4.6f)return -1;
 const float volts[]={3.20f,3.45f,3.60f,3.70f,3.78f,3.85f,3.95f,4.05f,4.15f,4.20f};
 const int pc[]={0,5,10,20,35,50,65,80,95,100};
 if(v<=volts[0])return 0;
 for(int i=1;i<10;i++)if(v<volts[i]){
  float pct=pc[i-1]+(v-volts[i-1])/(volts[i]-volts[i-1])*(pc[i]-pc[i-1]);
  return std::max(0,std::min(100,int(lroundf(pct))));
 }
 return 100;
}
struct Dwell {
 bool active=false;uint32_t since=0;
 bool held(bool condition,uint32_t now,uint32_t duration=2000){
  if(!condition){active=false;return false;}
  if(!active){active=true;since=now;}
  return uint32_t(now-since)>=duration;
 }
};
class Monitor {
 Reading value;bool started=false;float anchor=0,peak=0;uint32_t anchorAt=0;
 Dwell high,disconnect,resume;
public:
 Reading sample(float voltage,uint32_t now){
  if(percentage(voltage)<0){value=Reading();started=false;high=disconnect=resume=Dwell();return value;}
  value.voltage=started?(value.voltage+voltage)*0.5f:voltage;
  if(!started){started=true;anchor=peak=value.voltage;anchorAt=now;}
  if(uint32_t(now-anchorAt)>=2000){
   if(value.power==Power::Battery&&value.voltage-anchor>=0.030f&&value.voltage>=3.3f&&value.voltage<4.1f){value.power=Power::Charging;peak=value.voltage;}
   anchor=value.voltage;anchorAt=now;
  }
  if(high.held(value.voltage>=4.1f,now))value.power=Power::External;
  if(value.power!=Power::Battery){
   peak=std::max(peak,value.voltage);
   if(disconnect.held(value.voltage<4.06f&&peak-value.voltage>=0.065f,now)){
    value.power=Power::Battery;anchor=peak=value.voltage;anchorAt=now;high=disconnect=resume=Dwell();
   }else if(resume.held(value.power==Power::External&&value.voltage<4.07f,now))value.power=Power::Charging;
  }else{disconnect=resume=Dwell();}
  value.percent=percentage(value.voltage);return value;
 }
};
inline int segments(int percentage,Power power,uint32_t tick){
 if(percentage<0)return 0;
 if(power==Power::Charging)return 1+(tick/450)%4;
 return percentage==0?0:std::min(4,(percentage+24)/25);
}
}

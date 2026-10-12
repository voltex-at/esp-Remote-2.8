#include "battery_monitor.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace Battery;
 assert(percentage(NAN)==-1&&percentage(0)==-1&&percentage(5)==-1);
 assert(percentage(3.1f)==0&&percentage(4.2f)==100);
 for(int mv=2500;mv<4600;mv++)assert(percentage(mv/1000.0f)<=percentage((mv+1)/1000.0f));
 Monitor m;Reading r;
 for(uint32_t t=0;t<=8000;t+=500){r=m.sample(4.019f+(t%1000?0.003f:-0.003f),t);assert(r.power==Power::Battery);}
 // A sustained rise over two seconds starts charging animation below 4.1 V.
 for(uint32_t t=8500;t<=10500;t+=500)r=m.sample(4.019f+(t-8500)/2000.0f*0.065f,t);
 assert(r.power==Power::Charging);
 // A single high pulse cannot mark external power.
 r=m.sample(4.13f,11000);r=m.sample(4.08f,11500);assert(r.power!=Power::External);
 for(uint32_t t=12000;t<=16000;t+=500)r=m.sample(4.12f,t);
 assert(r.power==Power::External&&r.percent==percentage(r.voltage));
 assert(segments(r.percent,r.power,0)==segments(r.percent,r.power,1350));
 assert(segments(70,Power::Charging,0)!=segments(70,Power::Charging,450));
 // Threshold noise keeps the bolt on and the animation stopped.
 for(uint32_t t=16500;t<=22000;t+=500){r=m.sample(t%1000?4.096f:4.106f,t);assert(r.power==Power::External);}
 // A real unplug drop clears the bolt after the dwell, not after one sample.
 r=m.sample(4.019f,22500);assert(r.power==Power::External);
 for(uint32_t t=23000;t<=27000;t+=500)r=m.sample(4.019f,t);
 assert(r.power==Power::Battery);
 // USB at boot is recognized only after two seconds; timer wrap is safe.
 Monitor boot;uint32_t start=0xfffffc00u;
 for(uint32_t dt=0;dt<2000;dt+=500)assert(boot.sample(4.12f,start+dt).power==Power::Battery);
 assert(boot.sample(4.12f,start+2000).power==Power::External);
 assert(boot.sample(NAN,start+2500).percent==-1);
 assert(boot.sample(3.95f,start+3000).power==Power::Battery);
 puts("Battery: percentage curve, noise, two-second rise/dwell, charging animation, threshold hysteresis, unplug, invalid ADC and timer wrap OK.");
}

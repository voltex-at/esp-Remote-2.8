#include "runtime.h"
#include <stdio.h>
extern "C" int __lsan_is_turned_off(){return 1;}
AppState state;int stateMutex=1,ledTestLocks=0;static uint32_t now=0,color=0,calls=0;
uint32_t millis(){return now;}
void neopixelWrite(uint8_t pin,uint8_t r,uint8_t g,uint8_t b){assert(pin==42&&ledTestLocks==0);color=uint32_t(r)<<16|uint32_t(g)<<8|b;calls++;}
int main(){
 Light::begin();assert(calls==1&&color==SignalLed::scaled(SignalLed::Red,20));
 Light::tick(1000);assert(calls==1); // An unsuccessful boot must remain red.
 now=100;Light::ready();assert(calls==2&&color==SignalLed::scaled(SignalLed::Green,20));
 SignalLed::color(state.led,0x0088ff);state.led.brightness=100;
 Light::tick(200);Light::tick(1499);assert(calls==2);
 Light::tick(1600);assert(calls==3&&color==0x0088ff&&state.ledOutput==color);
 Light::tick(1700);assert(calls==3); // No redundant RMT transmission.
 auto&d=state.sensors[0];state.sensorCount=1;d.config.source=SensorRegistry::Source::Scd41;strcpy(d.config.address,"I2C:SCD");d.config.capabilities=7;d.updated[2]=1800;d.values[2]=700;SignalLed::select(state.led,d.config);state.led.mode=SignalLed::Mode::Co2;
 Light::tick(1800);assert(color==SignalLed::Green);d.values[2]=1200;Light::tick(1900);assert(color==SignalLed::Orange);d.values[2]=1500;Light::tick(1950);assert(color==SignalLed::Orange);Light::tick(2000);assert(color==SignalLed::Red);
 d.updated[2]=0;Light::tick(2100);assert(color==SignalLed::NoData);Light::tick(3000);assert(color==0&&state.ledOutput==0);
 state.led.mode=SignalLed::Mode::Fixed;SignalLed::color(state.led,0xabcd78);state.led.brightness=50;Light::tick(3100);assert(color==0x55663c&&state.ledOutput==color);
 state.led.mode=SignalLed::Mode::Off;Light::tick(3200);assert(color==0);assert(ledTestLocks==0);
 puts("LED driver: actual GPIO42 RGB arguments, startup failure/red, ready hold, 100ms rate limit, no duplicate frames, threshold/offline/off transitions and I/O outside state mutex OK.");
}

#include "runtime.h"
#include <esp32-hal-rgb-led.h>

namespace Light {
static SignalLed::Controller controller;
static uint32_t lastColor=0xffffffff,lastTick=0;
static bool running=false;
static void write(uint32_t rgb){
 if(rgb==lastColor)return;
 // Arduino-ESP32 2.0.17's RMT implementation sends GRB at 800 kHz.
 // No PWM channel, I2C access, extra task or frame-sized stack allocation.
 neopixelWrite(Pins::rgb,(rgb>>16)&255,(rgb>>8)&255,rgb&255);lastColor=rgb;
}
void begin(){running=false;controller.begin();write(SignalLed::scaled(SignalLed::Red,20));}
void ready(){controller.ready(millis());running=true;uint32_t rgb=SignalLed::scaled(SignalLed::Green,20);write(rgb);{Guard g(stateMutex);state.ledOutput=rgb;}}
void tick(uint32_t now){
 if(!running||now-lastTick<100)return;lastTick=now;
 uint32_t rgb;{Guard g(stateMutex);rgb=controller.sample(state.led,state.sensors,state.sensorCount,now);state.ledOutput=rgb;}
 write(rgb); // Only the loop task touches RMT; never hold stateMutex for I/O.
}
}

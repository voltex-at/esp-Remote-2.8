#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <assert.h>
#define OUTPUT 1
#define HIGH 1
#define LOW 0
#define MSBFIRST 1
#define SPI_MODE0 0
#define FSPI 0
namespace SpiSpy {
struct Byte {uint8_t value;bool data;};
inline uint8_t gpio[64]={};inline std::vector<Byte> bytes;
inline std::vector<unsigned> delays;inline bool transaction=false;
inline int pins[4]={};inline uint32_t hz=0;
}
inline void pinMode(int,int){}
inline void digitalWrite(int pin,int value){assert(pin>=0&&pin<64);SpiSpy::gpio[pin]=value;}
inline void delay(unsigned ms){SpiSpy::delays.push_back(ms);}

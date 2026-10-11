#pragma once
#include "Arduino.h"
#include <string.h>
struct SPISettings {uint32_t hz;SPISettings(uint32_t value,int order,int mode):hz(value){assert(order==MSBFIRST&&mode==SPI_MODE0);}};
class SPIClass {
public:
 explicit SPIClass(int host){assert(host==FSPI);}
 void begin(int clock,int input,int output,int cs){SpiSpy::pins[0]=clock;SpiSpy::pins[1]=input;SpiSpy::pins[2]=output;SpiSpy::pins[3]=cs;}
 void beginTransaction(const SPISettings&s){assert(!SpiSpy::transaction);SpiSpy::transaction=true;SpiSpy::hz=s.hz;}
 void endTransaction(){assert(SpiSpy::transaction&&SpiSpy::gpio[10]==HIGH);SpiSpy::transaction=false;}
 uint8_t transfer(uint8_t value){assert(SpiSpy::transaction&&SpiSpy::gpio[10]==LOW);SpiSpy::bytes.push_back({value,SpiSpy::gpio[46]==HIGH});return 0;}
 void writePixels(const void*data,uint32_t bytes){assert(bytes%2==0&&((uintptr_t)data&3)==0);const auto*p=(const uint8_t*)data;for(uint32_t i=0;i<bytes;i+=4){uint32_t word;memcpy(&word,p+i,4);uint32_t count=bytes-i>4?4:bytes-i;for(uint32_t j=0;j<count;j+=2){uint16_t color=word>>(j*8);transfer(color>>8);transfer(color&255);}}}
};

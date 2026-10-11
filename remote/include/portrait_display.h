#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "portrait_geometry.h"
// Small, synchronous RGB565 SPI transport. No LVGL or PSRAM dependency.
// ES3C28P hardware settings were cross-checked against Peppiuhr v1.19.15.
class PortraitDisplay:public Adafruit_GFX {
public:
 PortraitDisplay():Adafruit_GFX(Portrait::width,Portrait::height),spi_(FSPI){}
 void begin();
 void pushRect(int16_t x,int16_t y,uint16_t w,uint16_t h,const uint16_t*pixels);
 void drawPixel(int16_t x,int16_t y,uint16_t color) override;
 void writePixel(int16_t x,int16_t y,uint16_t color) override;
 void writeFillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t color) override;
 // GFX fillScreen calls fillRect, not writeFillRect. Override both API layers
 // so boot screens and diagnostics never open one SPI transaction per pixel.
 void fillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t color) override;
 void drawFastVLine(int16_t x,int16_t y,int16_t h,uint16_t color) override;
 void drawFastHLine(int16_t x,int16_t y,int16_t w,uint16_t color) override;
 void writeFastVLine(int16_t x,int16_t y,int16_t h,uint16_t color) override;
 void writeFastHLine(int16_t x,int16_t y,int16_t w,uint16_t color) override;
 // Only portrait 0/180 degrees is supported; never silently switch UI dimensions.
 void setRotation(uint8_t value) override;
private:
 SPIClass spi_;
 void window(uint16_t x,uint16_t y,uint16_t w,uint16_t h);
 void command(uint8_t value);
 void byte(uint8_t value);
 void word(uint16_t value);
 void select();
 void release();
};

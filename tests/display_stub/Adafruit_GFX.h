#pragma once
#include "Arduino.h"
class Adafruit_GFX {
protected:
 int16_t _width,_height;uint8_t rotation=0;
public:
 Adafruit_GFX(int16_t width,int16_t height):_width(width),_height(height){}
 virtual void drawPixel(int16_t,int16_t,uint16_t)=0;
 virtual void writePixel(int16_t x,int16_t y,uint16_t color){drawPixel(x,y,color);}
 virtual void writeFillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t c){for(int16_t j=0;j<h;j++)for(int16_t i=0;i<w;i++)writePixel(x+i,y+j,c);}
 virtual void setRotation(uint8_t value){rotation=value;}
 void setTextWrap(bool){}
 int16_t width()const{return _width;}int16_t height()const{return _height;}
 void fillScreen(uint16_t color){writeFillRect(0,0,_width,_height,color);}
};

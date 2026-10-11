#include "portrait_display.h"
#include "board_config.h"
#include <string.h>
namespace {
// Standard controller commands: reset, wake, 16-bit pixels, portrait+BGR,
// IPS inversion and display-on. Delays/settings match the working reference.
struct InitCommand {uint8_t command,value,bytes;uint16_t delayMs;};
constexpr InitCommand initialization[]={
 {0x01,0,0,120},{0x11,0,0,120},{0x3a,0x55,1,0},
 {0x36,LCD_MADCTL,1,0},{0x21,0,0,0},{0x29,0,0,20}
};
}
void PortraitDisplay::select(){spi_.beginTransaction(SPISettings(LCD_SPI_HZ,MSBFIRST,SPI_MODE0));}
void PortraitDisplay::release(){digitalWrite(Pins::lcdCs,HIGH);spi_.endTransaction();}
void PortraitDisplay::command(uint8_t value){digitalWrite(Pins::lcdDc,LOW);digitalWrite(Pins::lcdCs,LOW);spi_.transfer(value);digitalWrite(Pins::lcdCs,HIGH);}
void PortraitDisplay::byte(uint8_t value){digitalWrite(Pins::lcdDc,HIGH);digitalWrite(Pins::lcdCs,LOW);spi_.transfer(value);digitalWrite(Pins::lcdCs,HIGH);}
void PortraitDisplay::word(uint16_t value){spi_.transfer(value>>8);spi_.transfer(value&0xff);}
void PortraitDisplay::begin(){
 pinMode(Pins::lcdCs,OUTPUT);pinMode(Pins::lcdDc,OUTPUT);pinMode(Pins::backlight,OUTPUT);
 digitalWrite(Pins::lcdCs,HIGH);digitalWrite(Pins::lcdDc,HIGH);digitalWrite(Pins::backlight,LOW);
 spi_.begin(Pins::lcdSck,Pins::lcdMiso,Pins::lcdMosi,Pins::lcdCs);
 for(const auto&entry:initialization){select();command(entry.command);if(entry.bytes)byte(entry.value);release();if(entry.delayMs)delay(entry.delayMs);}
 rotation=LCD_ROTATION;_width=Portrait::width;_height=Portrait::height;
 setTextWrap(false);fillScreen(0x0000);
}
void PortraitDisplay::setRotation(uint8_t value){
 if(value!=0&&value!=2)return;select();command(0x36);byte(value==2?0x88:0x48);release();
 rotation=value;_width=Portrait::width;_height=Portrait::height;
}
void PortraitDisplay::window(uint16_t x,uint16_t y,uint16_t w,uint16_t h){
 command(0x2a);digitalWrite(Pins::lcdDc,HIGH);digitalWrite(Pins::lcdCs,LOW);word(x);word(x+w-1);digitalWrite(Pins::lcdCs,HIGH);
 command(0x2b);digitalWrite(Pins::lcdDc,HIGH);digitalWrite(Pins::lcdCs,LOW);word(y);word(y+h-1);digitalWrite(Pins::lcdCs,HIGH);
 command(0x2c);digitalWrite(Pins::lcdDc,HIGH);digitalWrite(Pins::lcdCs,LOW);
}
void PortraitDisplay::pushRect(int16_t x,int16_t y,uint16_t w,uint16_t h,const uint16_t*pixels){
 // LVGL flush rectangles are in bounds; rejecting invalid areas preserves stride.
 if(!pixels||!w||!h||x<0||y<0||(int32_t)x+w>Portrait::width||(int32_t)y+h>Portrait::height)return;
 select();window(x,y,w,h);
 // Arduino2 reads uint32_t words internally, even for the final single pixel.
 // Bulk-send even pixel counts only; do not read past a short/odd input buffer.
 const uint32_t count=(uint32_t)w*h,even=count&~1U;
 if(((uintptr_t)pixels&3)==0){if(even)spi_.writePixels(pixels,even*sizeof(uint16_t));}
 else {alignas(4) uint16_t block[128];for(uint32_t offset=0;offset<even;){uint32_t take=even-offset>128?128:even-offset;memcpy(block,pixels+offset,take*sizeof(uint16_t));spi_.writePixels(block,take*sizeof(uint16_t));offset+=take;}}
 if(count&1)word(pixels[count-1]);release();
}
void PortraitDisplay::drawPixel(int16_t x,int16_t y,uint16_t color){writePixel(x,y,color);}
void PortraitDisplay::writePixel(int16_t x,int16_t y,uint16_t color){
 if(x<0||y<0||x>=Portrait::width||y>=Portrait::height)return;
 select();window(x,y,1,1);word(color);release();
}
void PortraitDisplay::writeFillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t color){
 int32_t left=x,top=y,width=w,height=h;if(!Portrait::clip(left,top,width,height))return;
 alignas(4) uint16_t block[128];for(auto&p:block)p=color;uint32_t remaining=width*height;
 select();window(left,top,width,height);
 while(remaining){uint32_t count=remaining>128?128:remaining;spi_.writePixels(block,count*sizeof(uint16_t));remaining-=count;}
 release();
}

// Compile the actual portrait_display.cpp against a spy, not a second driver.
#include "portrait_display.h"
#include "board_config.h"
#include <stdio.h>
#include <assert.h>
extern "C" int __lsan_is_turned_off(){return 1;}
int main(){PortraitDisplay lcd;lcd.begin();assert(lcd.width()==240&&lcd.height()==320);
 const uint8_t commands[]={0x01,0x11,0x3a,0x36,0x21,0x29};unsigned index=0;
 for(const auto&b:SpiSpy::bytes)if(!b.data&&index<6){assert(b.value==commands[index]);index++;}assert(index==6);
 const uint8_t prefix[]={0x01,0x11,0x3a,0x55,0x36,LCD_ROTATION==2?0x88:0x48,0x21,0x29};
 for(unsigned i=0;i<sizeof prefix;i++)assert(SpiSpy::bytes[i].value==prefix[i]);
 assert(SpiSpy::delays.size()==3&&SpiSpy::delays[0]==120&&SpiSpy::delays[1]==120&&SpiSpy::delays[2]==20);
 assert(SpiSpy::pins[0]==12&&SpiSpy::pins[1]==13&&SpiSpy::pins[2]==11&&SpiSpy::pins[3]==10&&SpiSpy::hz==40000000);
 SpiSpy::bytes.clear();alignas(4) const uint16_t colors[]={0xf800,0x07e0,0x001f,0xffff};lcd.pushRect(1,2,2,2,colors);
 const uint8_t expected[]={0x2a,0,1,0,2,0x2b,0,2,0,3,0x2c,0xf8,0,0x07,0xe0,0,0x1f,0xff,0xff};
 assert(SpiSpy::bytes.size()==sizeof expected);for(unsigned i=0;i<sizeof expected;i++){assert(SpiSpy::bytes[i].value==expected[i]);assert(SpiSpy::bytes[i].data==(i!=0&&i!=5&&i!=10));}
 SpiSpy::bytes.clear();const uint16_t single[]={0x1234};lcd.pushRect(0,0,1,1,single);assert(SpiSpy::bytes.size()==13&&SpiSpy::bytes[11].value==0x12&&SpiSpy::bytes[12].value==0x34);
 SpiSpy::bytes.clear();alignas(4) const uint16_t odd[]={0xf800,0x07e0,0x001f};lcd.pushRect(0,0,3,1,odd);assert(SpiSpy::bytes.size()==17&&SpiSpy::bytes[15].value==0&&SpiSpy::bytes[16].value==0x1f);
 SpiSpy::bytes.clear();lcd.pushRect(0,0,2,1,colors+1);assert(SpiSpy::bytes.size()==15&&SpiSpy::bytes[11].value==7&&SpiSpy::bytes[12].value==0xe0&&SpiSpy::bytes[13].value==0&&SpiSpy::bytes[14].value==0x1f);
 SpiSpy::bytes.clear();lcd.pushRect(239,0,2,2,colors);lcd.pushRect(0,319,2,2,colors);lcd.pushRect(-1,0,1,1,colors);lcd.pushRect(0,0,1,1,nullptr);lcd.writePixel(240,0,1);lcd.setRotation(1);assert(SpiSpy::bytes.empty());
 lcd.writeFillRect(-1,-1,2,2,0x1234);assert(SpiSpy::bytes.size()==13&&SpiSpy::bytes[11].value==0x12&&SpiSpy::bytes[12].value==0x34);
 SpiSpy::bytes.clear();lcd.setRotation(2);assert(SpiSpy::bytes.size()==2&&SpiSpy::bytes[0].value==0x36&&SpiSpy::bytes[1].value==0x88);
 printf("LCD transport: actual driver, portrait %u, init/delays/pins, address windows, RGB565 byte order and CS/DC boundaries OK.\n",LCD_ROTATION);
}

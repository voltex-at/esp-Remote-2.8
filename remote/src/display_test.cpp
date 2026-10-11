// Standalone LCD/touch diagnostic. Excluded from the normal firmware build.
#include "portrait_display.h"
#include "board_config.h"
#include <Wire.h>
#include <esp_system.h>
static PortraitDisplay lcd;
static void pattern(){
 lcd.fillScreen(0x0000);const uint16_t colors[]={0xf800,0x07e0,0x001f};const char*names[]={"RED","GREEN","BLUE"};
 for(int i=0;i<3;i++){lcd.fillRect(i*80,48,80,112,colors[i]);lcd.setCursor(i*80+8,95);lcd.setTextColor(0xffff);lcd.setTextSize(1);lcd.print(names[i]);}
 lcd.drawRect(0,0,240,320,0xffff);lcd.setTextColor(0xffff);lcd.setTextSize(2);lcd.setCursor(8,12);lcd.print("LCD TEST");lcd.setTextSize(1);lcd.setCursor(8,33);lcd.print("240x320 / portrait");
 lcd.setCursor(8,177);lcd.print("TL (0,0) -> BR (239,319)");lcd.setCursor(8,202);lcd.print("Touch all four corners");lcd.setCursor(8,286);lcd.print("BOOT: redraw / USB: 115200");
}
void setup(){
 Serial.begin(115200);pinMode(Pins::ampEnable,OUTPUT);digitalWrite(Pins::ampEnable,HIGH);pinMode(Pins::boot,INPUT_PULLUP);
 Serial.printf("[LCD TEST] firmware=%s reset=%d\n",FIRMWARE_VERSION,(int)esp_reset_reason());
 lcd.begin();ledcSetup(0,5000,8);ledcAttachPin(Pins::backlight,0);ledcWrite(0,204);pattern();
 pinMode(Pins::touchReset,OUTPUT);digitalWrite(Pins::touchReset,LOW);delay(10);digitalWrite(Pins::touchReset,HIGH);delay(220);pinMode(Pins::touchIrq,INPUT_PULLUP);Wire.begin(Pins::sda,Pins::scl,100000);Wire.setTimeOut(20);
}
void loop(){
 static uint32_t report=0,lastTouch=0;static bool wasBoot=false;bool boot=digitalRead(Pins::boot)==LOW;if(boot&&!wasBoot)pattern();wasBoot=boot;
 if(millis()-report>2000){report=millis();lcd.fillRect(8,258,224,16,0x0000);lcd.setCursor(8,260);lcd.setTextSize(1);lcd.setTextColor(0x07e0);lcd.print("ALIVE ");lcd.print(report/1000);lcd.print("s / ");lcd.print(FIRMWARE_VERSION);
  Serial.printf("[LCD TEST] uptime=%lu portrait=%ux%u rotation=%u PSRAM=%u (not required by this test)\n",(unsigned long)(report/1000),Portrait::width,Portrait::height,LCD_ROTATION,ESP.getPsramSize());}
 Wire.beginTransmission(0x38);Wire.write(0x02);
 if(Wire.endTransmission(false)==0&&Wire.requestFrom((uint8_t)0x38,(uint8_t)5)==5){uint8_t n=Wire.read(),xh=Wire.read(),xl=Wire.read(),yh=Wire.read(),yl=Wire.read();int16_t x,y;
  if((n&15)>0&&(xh>>6)!=1&&Portrait::touch(((xh&15)<<8)|xl,((yh&15)<<8)|yl,x,y,LCD_ROTATION)){
   lcd.drawFastHLine(x-5,y,11,0xffff);lcd.drawFastVLine(x,y-5,11,0xffff);
   if(millis()-lastTouch>100){lastTouch=millis();lcd.fillRect(8,229,224,22,0x0000);lcd.setCursor(8,232);lcd.setTextSize(2);lcd.setTextColor(0xffff);lcd.print("X=");lcd.print(x);lcd.print(" Y=");lcd.print(y);Serial.printf("[TOUCH] x=%d y=%d\n",x,y);}
  }
 }delay(20);
}

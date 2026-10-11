#pragma once
#include <stddef.h>
#include <stdint.h>
// LCDWIKI ES3C28P / ESP32-S3 N16R8. GPIO are NOT physical pogo positions.
namespace Pins {
constexpr int lcdSck=12,lcdMosi=11,lcdMiso=13,lcdCs=10,lcdDc=46,backlight=45;
constexpr int sda=16,scl=15,touchReset=18,touchIrq=17;
constexpr int sdClk=38,sdCmd=40,sdD0=39,sdD1=41,sdD2=48,sdD3=47;
constexpr int mclk=4,bclk=5,lrclk=7,audioIn=6,audioOut=8,ampEnable=1;
constexpr int battery=9,boot=0,rgb=42;
}
constexpr float BATTERY_DIVIDER=2.0f; // Schematic equal 100k/100k; calibrate against meter.
constexpr float BATTERY_CALIBRATION=1.0f;
constexpr uint32_t PCM_RATE=16000;
constexpr uint16_t AUDIO_UDP_PORT=3333;
constexpr size_t MAX_JPEG_BYTES=100*1024;
constexpr uint32_t MAX_CLIP_MS=5*60*1000;
constexpr uint64_t SD_RESERVE_BYTES=64ULL*1024*1024;
#ifndef REMOTE_LCD_ROTATION
#define REMOTE_LCD_ROTATION 0
#endif
constexpr uint8_t LCD_ROTATION=REMOTE_LCD_ROTATION;
static_assert(LCD_ROTATION==0||LCD_ROTATION==2,"Only portrait rotations 0 and 2 are supported");
// ILI9341 rotation0: MX+BGR; rotation2: MY+BGR. Rotation1 in Peppiuhr is MV+BGR (0x28).
constexpr uint8_t LCD_MADCTL=LCD_ROTATION==2?0x88:0x48;
constexpr uint32_t LCD_SPI_HZ=40000000;
constexpr char FIRMWARE_VERSION[]="1.3.0";

#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <SD_MMC.h>
#include <Preferences.h>
#include "app_state.h"
#include "board_config.h"
#include "video_geometry.h"
extern AppState state;
extern SemaphoreHandle_t stateMutex,i2cMutex,sdMutex,frameMutex;
extern QueueHandle_t commands,mediaCommands;
extern uint16_t *videoPixels;
extern bool videoDirty;
struct Guard { SemaphoreHandle_t m;Guard(SemaphoreHandle_t x):m(x){xSemaphoreTake(m,portMAX_DELAY);}~Guard(){xSemaphoreGive(m);} };
AppState snapshot();
void message(const char*);
void alert(const char*);
void dispatch(Action,int,const char*);
namespace Board { void displayBegin();void bootStatus(const char*,const char*);void bootError(const char*,const char*);void bootReport();bool begin();void brightness(uint8_t);void battery();bool audioBegin();bool audioReady();void audioTask(void*);void audioSession(const char*,const char*,bool,bool);void readTouch(lv_indev_drv_t*,lv_indev_data_t*); }
namespace Sensors { void task(void*);void scan(); }
namespace Storage { bool begin();void logSample(const AppState&);void logEvent(const char*);bool available();String newPath(const char*,const char*);void files(); }
namespace Media { void task(void*); }

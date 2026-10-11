#pragma once
#include <lvgl.h>
#include "app_state.h"
namespace Ui {
void begin(Dispatch);
void show(Screen,int metric=0);
void update(const AppState&);
void frame(const uint16_t* pixels);
Screen current();
}

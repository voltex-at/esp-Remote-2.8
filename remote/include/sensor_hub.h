#pragma once
#include <stdint.h>
#include "app_state.h"
namespace SensorHub {
 bool begin();void task(void*);void command(const Command&);
 bool pause(uint32_t timeoutMs);void resume();bool reset();
}

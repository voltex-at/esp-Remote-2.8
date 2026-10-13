#pragma once
#include "app_state.h"
#include "board_config.h"
#include <assert.h>
extern AppState state;
extern int stateMutex,ledTestLocks;
uint32_t millis();
struct Guard {Guard(int m){assert(m==stateMutex);ledTestLocks++;}~Guard(){ledTestLocks--;}};
namespace Light {void begin();void ready();void tick(uint32_t);}

#pragma once
#include <stdio.h>
#define EXTERNAL 0 // Arduino ADC reference macro
struct SerialStub {void println(const char*){}template<typename...T>void printf(const char*,T...){} };
extern SerialStub Serial;

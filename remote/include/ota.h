#pragma once
#include "ota_policy.h"
namespace Ota {Status status();bool busy();bool installing();bool check();bool install(const char*expectedVersion);void reject(Error);}

#pragma once
#include <stddef.h>
#include <stdint.h>
namespace TlsMemory {
struct Stats {uint32_t external=0,internal=0,failed=0;size_t lastFailed=0;};
// Install the supported mbedTLS allocator once, before network tasks start.
bool begin();
Stats stats();
void report(const char*phase);
}

#include "tls_memory.h"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <mbedtls/platform.h>
#include <atomic>
#include <stdint.h>
namespace TlsMemory {namespace {
constexpr uint32_t TLS_PSRAM_CAPS=MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT;
constexpr uint32_t TLS_INTERNAL_CAPS=MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT;
std::atomic<uint32_t> externalCount{0},internalCount{0},failedCount{0};
std::atomic<size_t> lastFailed{0};
bool ready=false;
void*allocate(size_t n,size_t size){
 if(!n||!size)return nullptr;
 if(n>SIZE_MAX/size){failedCount.fetch_add(1,std::memory_order_relaxed);lastFailed.store(SIZE_MAX,std::memory_order_relaxed);return nullptr;}
 const size_t bytes=n*size;
 // ESP-IDF's prebuilt SDK forces TLS into internal RAM by default. The
 // 16KB record buffers belong in PSRAM; keep small allocations internal when
 // available, with the other bank as a fallback. calloc must zero every byte.
 const bool large=bytes>=1024;
 void*p=heap_caps_calloc(n,size,large?TLS_PSRAM_CAPS:TLS_INTERNAL_CAPS);
 bool external=large;
 if(!p){p=heap_caps_calloc(n,size,large?TLS_INTERNAL_CAPS:TLS_PSRAM_CAPS);external=!large;}
 if(p){(external?externalCount:internalCount).fetch_add(1,std::memory_order_relaxed);return p;}
 failedCount.fetch_add(1,std::memory_order_relaxed);lastFailed.store(bytes,std::memory_order_relaxed);return nullptr;
}
void release(void*p){heap_caps_free(p);}
}
bool begin(){
 if(ready)return true;
 if(!heap_caps_get_total_size(TLS_PSRAM_CAPS)){Serial.println("[TLS memory] PSRAM unavailable");return false;}
 // Do not change callbacks temporarily around one request: mbedTLS allocation
 // is process-wide. Install before workers/Wi-Fi and leave the matching free
 // callback in place for objects whose lifetime spans requests.
 int result=mbedtls_platform_set_calloc_free(allocate,release);
 if(result){Serial.printf("[TLS memory] allocator setup failed: %d\n",result);return false;}
 ready=true;report("allocator ready");return true;
}
Stats stats(){Stats s;s.external=externalCount.load(std::memory_order_relaxed);s.internal=internalCount.load(std::memory_order_relaxed);s.failed=failedCount.load(std::memory_order_relaxed);s.lastFailed=lastFailed.load(std::memory_order_relaxed);return s;}
void report(const char*phase){auto s=stats();Serial.printf("[TLS memory] %s internal_free=%u internal_largest=%u psram_free=%u psram_largest=%u external_allocs=%u internal_allocs=%u failed=%u last_failed=%u\n",phase,unsigned(heap_caps_get_free_size(TLS_INTERNAL_CAPS)),unsigned(heap_caps_get_largest_free_block(TLS_INTERNAL_CAPS)),unsigned(heap_caps_get_free_size(TLS_PSRAM_CAPS)),unsigned(heap_caps_get_largest_free_block(TLS_PSRAM_CAPS)),unsigned(s.external),unsigned(s.internal),unsigned(s.failed),unsigned(s.lastFailed));}
}

#include "tls_memory.h"
#include "Arduino.h"
#include "esp_heap_caps.h"
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
extern "C" int __lsan_is_turned_off(){return 1;} // restricted executor has no /proc; successful allocations are counted/freed below
SerialStub Serial;
static void*(*tlsCalloc)(size_t,size_t);static void(*tlsFree)(void*);
static int installs=0,setupResult=0;static bool psram=true,failInternal=false,failExternal=false;
static uint32_t lastCaps=0;static unsigned calls=0,frees=0;
int mbedtls_platform_set_calloc_free(void*(*a)(size_t,size_t),void(*f)(void*)){installs++;if(setupResult)return setupResult;tlsCalloc=a;tlsFree=f;return 0;}
void*heap_caps_calloc(size_t n,size_t s,uint32_t cap){calls++;lastCaps=cap;if((cap&MALLOC_CAP_INTERNAL)&&failInternal)return nullptr;if((cap&MALLOC_CAP_SPIRAM)&&failExternal)return nullptr;return std::calloc(n,s);}
void heap_caps_free(void*p){if(p)frees++;std::free(p);}
size_t heap_caps_get_total_size(uint32_t cap){return cap&MALLOC_CAP_SPIRAM?(psram?8*1024*1024:0):320*1024;}
size_t heap_caps_get_free_size(uint32_t cap){return heap_caps_get_total_size(cap)/2;}
size_t heap_caps_get_largest_free_block(uint32_t cap){return heap_caps_get_free_size(cap)/2;}
static void zeroed(void*p,size_t n){assert(p);for(size_t i=0;i<n;i++)assert(((unsigned char*)p)[i]==0);tlsFree(p);}
int main(){
 psram=false;assert(!TlsMemory::begin()&&installs==0);
 psram=true;setupResult=-1;assert(!TlsMemory::begin()&&installs==1);
 setupResult=0;assert(TlsMemory::begin()&&installs==2&&tlsCalloc&&tlsFree);assert(TlsMemory::begin()&&installs==2);
 zeroed(tlsCalloc(2,16384),32768);assert(lastCaps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
 zeroed(tlsCalloc(1,256),256);assert(lastCaps==(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
 // Reproduce the device condition: internal heap cannot satisfy TLS's record
 // buffer. A strict internal allocator fails; the installed allocator succeeds.
 failInternal=true;assert(!heap_caps_calloc(1,16384,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));zeroed(tlsCalloc(1,16384),16384);assert(lastCaps&(MALLOC_CAP_SPIRAM));
 zeroed(tlsCalloc(1,256),256);assert(lastCaps&(MALLOC_CAP_SPIRAM));
 failInternal=false;failExternal=true;zeroed(tlsCalloc(1,16384),16384);assert(lastCaps&(MALLOC_CAP_INTERNAL));
 failInternal=true;auto before=TlsMemory::stats();assert(!tlsCalloc(1,16384));auto failed=TlsMemory::stats();assert(failed.failed==before.failed+1&&failed.lastFailed==16384);
 unsigned count=calls;assert(!tlsCalloc(SIZE_MAX,2)&&calls==count);assert(TlsMemory::stats().lastFailed==SIZE_MAX);
 assert(!tlsCalloc(0,9)&&!tlsCalloc(9,0)&&calls==count);tlsFree(nullptr);
 auto s=TlsMemory::stats();assert(s.external==3&&s.internal==2&&frees==5);TlsMemory::report("test");
 puts("TLS memory: 16KB buffers survive internal-RAM exhaustion; calloc zeroing, fallback banks, failures, overflow, free and one-time installation OK.");
}

#ifndef LV_CONF_H
#define LV_CONF_H
#define LV_COLOR_DEPTH 16
#define LV_USE_LARGE_COORD 1
#define LV_COLOR_16_SWAP 0
#ifdef ARDUINO
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE "esp_heap_caps.h"
#define LV_MEM_CUSTOM_ALLOC(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define LV_MEM_CUSTOM_FREE heap_caps_free
#define LV_MEM_CUSTOM_REALLOC(ptr,size) heap_caps_realloc((ptr),(size),MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#else
#define LV_MEM_SIZE (1024U * 1024U)
#endif
#define LV_TICK_CUSTOM 0
#define LV_USE_LOG 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_CHART 1
#define LV_USE_QRCODE 1
#define LV_USE_SNAPSHOT 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#endif

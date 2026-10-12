#pragma once
#include <stddef.h>
#include <stdint.h>
#define MALLOC_CAP_8BIT 1U
#define MALLOC_CAP_INTERNAL 2U
#define MALLOC_CAP_SPIRAM 4U
void*heap_caps_calloc(size_t,size_t,uint32_t);
void heap_caps_free(void*);
size_t heap_caps_get_total_size(uint32_t);
size_t heap_caps_get_free_size(uint32_t);
size_t heap_caps_get_largest_free_block(uint32_t);

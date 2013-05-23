#ifndef _TAPIOS_PMM_H_
#define _TAPIOS_PMM_H_

#include <stdint.h>
#include "util.h"

#define BITMAP_SIZE 32768

uint32_t bitmap[BITMAP_SIZE];

void setup_bitmap(void);
bool is_free_page(physaddr_t addr);
physptr_t* kalloc_page_frame();
void kfree_page_frame(physptr_t* ptr);

#endif

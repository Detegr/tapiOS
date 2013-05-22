#include "bitmap.h"
#include "util.h"
#include "video.h"

extern uint8_t* __kernel_end;
uint32_t* page_directory=(uint32_t*)0xC0101000;

void setup_bitmap(void)
{
	for(int i=0; i<BITMAP_SIZE; ++i) bitmap[i]=0;
	for(int i=0; i<32; ++i) bitmap[i]=0xFFFFFFFF; // Kernel mapping
}

bool is_free_page(physaddr_t addr)
{
	unsigned i=addr>>17;
	unsigned offset=0x20000 - (addr % 0x10000);

	return (bitmap[i] & offset) == 0;
}

physptr_t* kalloc_page_frame(void)
{
	for(int i=0; i<BITMAP_SIZE; ++i)
	{
		if(bitmap[i] != 0xFFFFFFFF)
		{
		}
	}
	return NULL;
}

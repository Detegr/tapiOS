#include "bitmap.h"

void setup_bitmap(void)
{
	for(int i=0; i<BITMAP_SIZE; ++i) bitmap[i]=0;
	for(int i=0; i<32; ++i) bitmap[i]=1; // Kernel mapping
}

bool is_free_page(physptr_t addr)
{
	unsigned i=addr>>17;
	unsigned offset=addr % 0x1000;

	return (bitmap[i] & offset) == 0;
}

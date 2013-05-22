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
	unsigned bitoffset=addr % 0x10000;
	unsigned offset=bitoffset ? 31 - 0x20000 - bitoffset : 31;

	return (bitmap[i] & (1 << offset)) == 0;
}

physptr_t* kalloc_page_frame(void)
{
	for(int i=0; i<BITMAP_SIZE; ++i)
	{
		if(bitmap[i] != 0xFFFFFFFF)
		{
			for(int bit=31, j=0; bit; bit--, j++)
			{
				if(((bitmap[i] >> bit) & 0x1) == 0)
				{
					bitmap[i] |= 1<<(31-j);
					return __kernel_end + (i * 0x1000 * 32) + (j * 0x1000);
				}
			}
		}
	}
	return NULL;
}

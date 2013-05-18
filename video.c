#include "video.h"

volatile unsigned char* video = (unsigned char*)0xC00B8000;

static unsigned char row=0;
static unsigned char line=0;

void cls(void)
{
	int x,y;
	for(x=0; x<0xA0; ++x)
	{
		for(y=0; y<0x32; ++y)
		{
			*(video+(y*0xA0)+x)=0;
			*(video+(y*0xA0)+x+1)=0;
		}
	}
	row=line=0;
}

void printkchar(const char c, uint8_t color)
{
	if(c=='\n')
	{
		row=0;
		line++;
		return;
	}
	*(video+(line*0xA0)+row)=c;
	*(video+(line*0xA0)+row+1)=color;
	row+=2;
	if(row>=0xA0)
	{
		row=0;
		line++;
	}
}

void printkc(const char* str, uint8_t color)
{
	const char* s;
	for(s=str; *s; s++)
	{
		printkchar(*s, color);
	}
}

char halfbytetohex(uint8_t b)
{
	char hex[]={'0','1','2','3','4','5','6','7',
				'8','9','A','B','C','D','E','F'};
	return hex[b];
}

void printix(uint32_t x)
{
	printk("0x");
	int i;
	for(i=28; i>=0; i-=4)
	{
		printkchar(halfbytetohex((x>>i) & 0x0F), 0x07);
	}
}


inline void printk(const char* str) { printkc(str, 0x07); }
inline void print_startup_info(const char* section, const char* msg)
{
	printkc("[", 0x0F);
	printkc(section, 0x09);
	printkc("] ", 0x0F);
	printk(msg);
}

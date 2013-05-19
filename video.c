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

void printkc(const char* str, uint8_t color)
{
	const char* s;
	for(s=str; *s; s++)
	{
		char c=*s;
		if(c=='\n')
		{
			row=0;
			line++;
			continue;
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
}

inline void printk(const char* str) { printkc(str, 0x07); }

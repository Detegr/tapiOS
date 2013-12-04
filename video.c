#include "video.h"
#include "util.h"
#include <stdarg.h>

volatile unsigned char* video = (unsigned char*)0xC00B8000;

static uint8_t col=0;
static uint8_t row=0;

void hide_cursor(void)
{
	outw(0x3D4,0x200A);
	outw(0x3D4,0xB);
}

void set_cursor(uint8_t row, uint8_t col)
{
	uint16_t pos=(row*80) + col;
	outb(0x3D4, 15); // Number of col register on vga controller
	outb(0x3D5, pos & 0xFF); // Low byte of pos
	outb(0x3D4, 14); // Number of col register on vga controller
	outb(0x3D5, (pos >> 8) & 0xFF); // High byte of pos
}

void cls(void)
{
	int x,y;
	for(y=0; y<50; ++y)
	{
		for(x=0; x<160; x+=2)
		{
			*(video+(y*160)+x)=' ';
			*(video+(y*160)+x+1)=0x07;
		}
	}
	col=row=0;
}

void printkchar(const char c, uint8_t color)
{
	if(c=='\n')
	{
		col=0;
		row++;
		return;
	}
	*(video+(row*160)+col)=c;
	*(video+(row*160)+col+1)=color;
	col+=2;
	if(col>=160)
	{
		col=0;
		row++;
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

void printi(uint32_t i)
{
	static char buf[12]; // uint32_t + \0
	char* p=&buf[11];
	do {
		*(--p) = '0' + i%10;
		i /= 10;
	} while(i!=0);
	printk(p);
}

inline void printk(const char* str) { printkc(str, 0x07); }
inline void print_startup_info(const char* section, const char* msg)
{
	printkc("[", 0x0F);
	printkc(section, 0x09);
	printkc("] ", 0x0F);
	printk(msg);
}

void kprintf(const char* fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	for(const char* p=fmt; *p; ++p)
	{
		if(*p != '%') printkchar(*p, 0x07);
		else
		{
			switch(*(++p))
			{
				case 'x':
				{
					int i=va_arg(argp, int);
					printix(i);
					break;
				}
				case 'c':
				{
					char c=va_arg(argp, int);
					printkchar(c, 0x07);
					break;
				}
				case 'd':
				{
					int i=va_arg(argp, int);
					printi(i);
					break;
				}
				case 's':
				{
					char* p=va_arg(argp, char*);
					printk(p);
					break;
				}
				case '%':
				{
					printkchar('%', 0x07);
					break;
				}
			}
		}
	}
	va_end(argp);
	set_cursor(row, col/2);
}

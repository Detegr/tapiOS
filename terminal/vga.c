#include "vga.h"
#include <util/util.h>
#include <stdarg.h>

volatile uint8_t* video = (uint8_t*)0xC00B8000;

static uint8_t col=0;
static uint8_t row=25;

void hide_cursor(void)
{
	uint16_t pos=(100*80) + 200;
	outb(0x3D4, 15); // Number of col register on vga controller
	outb(0x3D5, pos & 0xFF); // Low byte of pos
	outb(0x3D4, 14); // Number of col register on vga controller
	outb(0x3D5, (pos >> 8) & 0xFF); // High byte of pos
}

void move_cursor(uint8_t rows, uint8_t cols)
{
	uint16_t pos=(row * 80) + (rows * 80) + col + (cols * 2);
	outb(0x3D4, 15); // Number of col register on vga controller
	outb(0x3D5, pos & 0xFF); // Low byte of pos
	outb(0x3D4, 14); // Number of col register on vga controller
	outb(0x3D5, (pos >> 8) & 0xFF); // High byte of pos
	row=row+rows; col+=(cols*2);
	if(col > 160)
	{
		row++;
		col = col % 160;
	}
}

void set_cursor(uint8_t newrow, uint8_t newcol)
{
	uint16_t pos=(newrow*80) + newcol;
	outb(0x3D4, 15); // Number of col register on vga controller
	outb(0x3D5, pos & 0xFF); // Low byte of pos
	outb(0x3D4, 14); // Number of col register on vga controller
	outb(0x3D5, (pos >> 8) & 0xFF); // High byte of pos
	row=newrow; col=newcol*2;
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

void cls_from_cursor_down(void)
{
	for(int y=row; y<50; ++y)
	{
		for(int x=col; x<160; x+=2)
		{
			*(video+(y*160)+x)=' ';
			*(video+(y*160)+x+1)=0x07;
		}
	}
}

static void printchar(const char c, uint8_t color)
{
	if(!c) return;
	if(c=='\n')
	{
		col=0;
		row++;
	}
	else if(c=='\r')
	{
		col=0;
	}
	else
	{
		*(video+(row*160)+col)=c;
		*(video+(row*160)+col+1)=color;
		col+=2;
		if(col>=160)
		{
			col=0;
			row++;
		}
	}
	if(row>24)
	{ // Scroll the buffer
		row=24;
		memmove((uint8_t*)video, ((uint8_t*)video)+160, 25*160);
	}
}

void kprintc(const char c)
{
	printchar(c, 0x07);
}

static void print(const char* str, uint8_t color)
{
	const char* s;
	for(s=str; *s; s++)
	{
		printchar(*s, color);
	}
}

static char halfbytetohex(uint8_t b)
{
	char hex[]={'0','1','2','3','4','5','6','7',
				'8','9','A','B','C','D','E','F'};
	return hex[b];
}

static void printix(uint32_t x, int color)
{
	print("0x", color);
	int i;
	for(i=28; i>=0; i-=4)
	{
		printchar(halfbytetohex((x>>i) & 0x0F), color);
	}
}

static void printi(uint32_t i, int color)
{
	static char buf[12]; // uint32_t + \0
	char* p=&buf[11];
	do {
		*(--p) = '0' + i%10;
		i /= 10;
	} while(i!=0);
	print(p, color);
}

inline void print_startup_info(const char* section, bool ok)
{
	kprintf("%s %@[%@%s%@]\n", section, 0x0F, ok?0x09:0x0C, ok?"OK":"FAIL", 0x0F);
}

void kprintf(const char* fmt, ...)
{
	int color=0x07;

	va_list argp;
	va_start(argp, fmt);
	for(const char* p=fmt; *p; ++p)
	{
		if(*p != '%') printchar(*p, color);
		else
		{
			switch(*(++p))
			{
				case 'x':
				{
					int i=va_arg(argp, int);
					printix(i, color);
					break;
				}
				case 'c':
				{
					int c=va_arg(argp, int);
					printchar((char)c, color);
					break;
				}
				case 'd':
				{
					int i=va_arg(argp, int);
					printi(i, color);
					break;
				}
				case 's':
				{
					char* p=va_arg(argp, char*);
					print(p, color);
					break;
				}
				case '%':
				{
					printchar('%', color);
					break;
				}
				case '@':
				{
					color=va_arg(argp, int);
					break;
				}
			}
		}
	}
	va_end(argp);
}

void delete_last_char(int minrow, int mincol)
{
	if(row==0 && col==0) return;
	if(col==0 && row>minrow) {row--;col=158;}
	else if(col>0 && col>mincol) col-=2;
	kprintf("%c", ' ');
	if(col==0 && row>minrow) {row--;col=158;}
	else if(col>0 && col>mincol) col-=2;
}

void update_cursor(void)
{
	set_cursor(row, col/2);
}

int get_cursor_row(void)
{
	return row;
}

int get_cursor_col(void)
{
	return col;
}

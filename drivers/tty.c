#include "tty.h"
#include <fs/devfs.h>
#include <terminal/vga.h>
#include <util/util.h>
#include <util/scancodes.h>
#include <task/process.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <drivers/keyboard.h>
#include <sys/poll.h>

#define TTY_BUFFER_SIZE 1024
#define CANONICAL ((tty_termios.c_lflag & ICANON) == ICANON)

static bool bold=false;

static struct file_actions tty_fact;
static struct inode_actions tty_iact;
static struct termios tty_termios; // TODO: What about multiple terminals?

static char inbuf[TTY_BUFFER_SIZE];
static int inbuf_size=0;
static char outbuf[TTY_BUFFER_SIZE];
static int outbuf_size=0;

static const char *handle_escape(const char *str)
{
	unsigned int row, col;
	char *outstr;
	int elems=ksscanf(str, "\033[%u;%uH", &outstr, &row, &col);
	if(elems==2 && outstr)
	{
		if(row == 0) row++;
		if(col == 0) col++;
		set_cursor(row-1, col-1);
		return outstr;
	}
	elems=ksscanf(str, "\033[%u;%ur", &outstr, &row, &col);
	if(elems==2 && outstr)
	{
		// Set scrolling area to (row, col)
		return outstr;
	}
	elems=ksscanf(str, "\033[%uC", &outstr, &col);
	if(elems==1 && outstr)
	{
		move_cursor(0, col);
		return outstr;
	}
	elems=ksscanf(str, "\033[%uJ", &outstr, &col);
	if(elems==1 && outstr)
	{
		cls();
		return outstr;
	}
	elems=ksscanf(str, "\033[%um", &outstr, &col);
	if(elems==1 && outstr)
	{
		if(col == 1) bold=true;
		else if(col==0) bold=false;
		return outstr;
	}
	if(strncmp(str, "\033[H", 3) == 0)
	{
		set_cursor(0, 0);
		return str+3;
	}
	if(strncmp(str, "\033[J", 3) == 0)
	{
		cls_from_cursor_down();
		return str+3;
	}
	if(strncmp(str, "\033[K", 3) == 0)
	{
		cls_from_cursor_to_eol();
		return str+3;
	}
	if(strncmp(str, "\033[m", 3) == 0)
	{
		// Turn off character attributes
		bold=false;
		return str+3;
	}
	if(strncmp(str, "\033[?1l", 5) == 0)
	{
		return str+5;
	}
	if(strncmp(str, "\033=", 2) == 0)
	{
		// Enter alternate keypad mode
		return str+2;
	}
	if(strncmp(str, "\033>", 2) == 0)
	{
		// Exit alternate keypad mode
		return str+2;
	}
	if(strncmp(str, "\033M", 2) == 0)
	{
		scroll(-1);
		return str+2;
	}
	if(strncmp(str, "\033D", 2) == 0)
	{
		scroll(1);
		return str+2;
	}
	return str;
}

ssize_t tty_write(struct file *f, const void *data, size_t count)
{
	int i=0;
	for(const char *p=data; i<count; ++i)
	{
		if(*p == '\033')
		{
			const char *pp=handle_escape(p);
			if(pp!=p)
			{
				int move=pp-p-1;
				if(move>0) i+=move;
				p=pp;
				continue;
			}
		}
		else if(*p == '\017' || *p == '\016') {p++; continue;} // Some scancode settings, ignore
		kprintca(*(p++), bold);
	}
	update_cursor();
	return count;
}

int32_t tty_read(struct file *f, void *top, uint32_t count)
{
	char *to=top;
	int i;
	int startrow=get_cursor_row();
	int startcol=get_cursor_col();
	for(i=0; i<count;)
	{
		int r=kbd_read(NULL, to+i, 1);
		if(to[i] == CHAR_BACKSPACE && CANONICAL)
		{
			if(i==0) continue;
			to[--i]=0;
			delete_last_char(startrow, startcol);
			update_cursor();
			--i;
		}
		else if((tty_termios.c_lflag & ECHO) == ECHO)
		{
			kprintc(to[i]);
			update_cursor();
		}
		if(!CANONICAL) return r;
		if(to[i] == '\n') return i+r;
		i+=r;
	}
	return i;
}

int32_t tty_ioctl(struct file *f, int cmd, void *arg)
{
	switch(cmd)
	{
		case TCGETATTR:
		{
			struct termios *t=arg;
			t->c_iflag=tty_termios.c_iflag;
			t->c_oflag=tty_termios.c_oflag;
			t->c_lflag=tty_termios.c_lflag;
			t->c_cflag=tty_termios.c_cflag;
			return 0;
		}
		case TCSETATTR:
		{
			struct termios *t=arg;
			tty_termios.c_iflag=t->c_iflag;
			tty_termios.c_oflag=t->c_oflag;
			tty_termios.c_lflag=t->c_lflag;
			tty_termios.c_cflag=t->c_cflag;
			return 0;
		}
	}
	return -EBADF;
}

static int32_t tty_poll(struct file *f, uint16_t events, uint16_t *revents)
{
	if(events & POLLIN)
	{
		if(kbd_hasdata())
		{
			*revents|=POLLIN;
			return 1;
		}
	}
	return 0;
}

static void set_initial_termios(void)
{
	tty_termios.c_iflag=INLCR|IXON;
	tty_termios.c_oflag=OPOST/*|ONLCR*/;
	tty_termios.c_lflag=ISIG|ICANON|ECHO|ECHOE|ECHOK|IEXTEN;
	tty_termios.c_cflag=B38400|CS8|CREAD|HUPCL;
}

void register_tty_driver(void)
{
	memset(inbuf, 0, TTY_BUFFER_SIZE);
	memset(outbuf, 0, TTY_BUFFER_SIZE);
	memset(&tty_fact, 0, sizeof(tty_fact));
	memset(&tty_iact, 0, sizeof(tty_iact));

	tty_fact.read=&tty_read;
	tty_fact.write=&tty_write;
	tty_fact.ioctl=&tty_ioctl;
	tty_fact.poll=&tty_poll;

	set_initial_termios();

	register_device(TERMINAL, &tty_fact, &tty_iact);
	struct inode *devfs=vfs_search((struct inode*)root_fs, "/dev");
	devfs_mknod(devfs, "terminal", TERMINAL, 0);
}

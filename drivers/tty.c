#include "tty.h"
#include <fs/devfs.h>
#include <terminal/vga.h>
#include <util/util.h>
#include <util/scancodes.h>
#include <task/process.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <drivers/keyboard.h>

#define TTY_BUFFER_SIZE 1024

static struct file_actions tty_fact;
static struct inode_actions tty_iact;
static struct termios tty_termios; // TODO: What about multiple terminals?

static char inbuf[TTY_BUFFER_SIZE];
static int inbuf_size=0;
static char outbuf[TTY_BUFFER_SIZE];
static int outbuf_size=0;

static char *handle_escape(char *str)
{
	unsigned int row, col;
	char *outstr;
	int elems=ksscanf(str, "\033[%u;%uH", &outstr, &row, &col);
	if(elems==2 && outstr)
	{
		set_cursor(row-1, col-1);
		return outstr;
	}
	elems=ksscanf(str, "\033[%uC", &outstr, &col);
	if(elems==1 && outstr)
	{
		move_cursor(0, col);
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
	if(strncmp(str, "\033[m", 3) == 0)
	{
		// Turn off character attributes
		return str+3;
	}
	if(strncmp(str, "\033[?1l", 5) == 0)
	{
		return str+5;
	}
	if(strncmp(str, "\033>", 2) == 0)
	{
		return str+2;
	}
	return str;
}

int32_t tty_write(struct file *f, void *data, uint32_t count)
{
	for(char *p=data, i=0; i<count; ++i)
	{
		if(*p == '\033')
		{
			char *pp=handle_escape(p);
			if(pp!=p) {p=pp; continue;}
		}
		kprintc(*(p++));
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
		if(to[i] == CHAR_BACKSPACE)
		{
			if(i==0) continue;
			to[--i]=0;
			delete_last_char(startrow, startcol);
			update_cursor();
			--i;
		}
		else if(/*echo*/1) {kprintc(to[i]); update_cursor();}
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
			return 0;
		}
	}
	return -EBADF;
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

	tty_fact.read=&tty_read;
	tty_fact.write=&tty_write;
	tty_fact.ioctl=&tty_ioctl;

	set_initial_termios();

	register_device(TERMINAL, &tty_fact, &tty_iact);
	struct inode *devfs=vfs_search((struct inode*)root_fs, "/dev");
	devfs_mknod(devfs, "terminal", TERMINAL, 0);
}

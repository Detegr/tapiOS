#include <fs/devfs.h>
#include <terminal/vga.h>
#include <util/util.h>
#include <util/scancodes.h>
#include <task/process.h>

#include "keyboard.h"

struct file_actions kbd_fact;
struct inode_actions kbd_iact;

int32_t kbd_read(struct file *f, void *top, uint32_t count)
{
	uint8_t *to=top;
	if(count==1) return 0;
	struct process* p=find_active_process();
	if(!p) PANIC();
	memset(p->keybuf, 0, 256);
	uint32_t bufptr=0;
	for(int i=0; i<256; ++i)
	{
		if(p->stdoutbuf[i] == 0)
		{
			bufptr=i;
			break;
		}
	}
	uint32_t j=0;
	if(bufptr>0)
	{
		if(bufptr>count)
		{
			memcpy(to, p->stdoutbuf, count);
			memmove(p->stdoutbuf, p->stdoutbuf+count, 256-count);
			return count;
		}
		else
		{
			memcpy(to, p->stdoutbuf, count);
			memset(p->stdoutbuf, 0, 256);
			return count;
		}
	}
	int prevkp=p->keyp;
	int row=get_cursor_row();
	int col=get_cursor_col();
	update_cursor();
	while(true)
	{
		prevkp=p->keyp;
		__asm__ volatile("sti;hlt");
		if(p->keyp-prevkp == 0) continue;
		for(int i=prevkp; i<=p->keyp; ++i)
		{
			char c=char_for_scancode(p->keybuf[i]);
			if(c==CHAR_UP || c==CHAR_UNHANDLED) continue;
			else if(c==CHAR_BACKSPACE) delete_last_char(row, col);
			else kprintc(c);
			update_cursor();
			if(c=='\n') goto done;
		}
	}
done:
	hide_cursor();
	int i=0;
	for(i=0; i<p->keyp; ++i)
	{
		char c=char_for_scancode(p->keybuf[i]);
		if(c==CHAR_UNHANDLED || c==CHAR_UP) continue;
		else if(c=='\n')
		{
			p->stdoutbuf[bufptr++]=c;
			break;
		}
		else if(c==CHAR_BACKSPACE)
		{
			if(bufptr==0) continue;
			p->stdoutbuf[bufptr--]=0;
		}
		else
		{
			p->stdoutbuf[bufptr++]=c;
		}
	}
	for(j=0; j<bufptr; ++j)
	{
		if(j==count) break;
		to[j]=p->stdoutbuf[j];
	}
	memmove(p->keybuf, p->keybuf+i, 256-i);
	p->keyp-=i;
	memset(p->keybuf+i, 0, 256-i);
	memmove(p->stdoutbuf, p->stdoutbuf+j, 256-j);
	return j;
}


void register_kbd_driver(void)
{
	kbd_fact.read=&kbd_read;

	register_device(KEYBOARD, &kbd_fact, &kbd_iact);
	struct inode *devfs=vfs_search((struct inode*)root_fs, "/dev");
	devfs_mknod(devfs, "keyboard", KEYBOARD, 0);
}

#include <fs/devfs.h>
#include <terminal/vga.h>
#include <util/util.h>
#include <util/scancodes.h>
#include <task/process.h>

#include "keyboard.h"

#define KBD_BUFFER_SIZE 256

static struct file_actions kbd_fact;
static struct inode_actions kbd_iact;
static uint8_t kbd_buffer[KBD_BUFFER_SIZE]; // TODO: Multiple devices again... How?
static uint8_t kbd_buf_size=0;

void kbd_buffer_push(uint8_t c)
{
	if(kbd_buf_size==255) return;
	kbd_buffer[kbd_buf_size++]=c;
}

static char kbd_buffer_pop(void)
{
	return kbd_buffer[--kbd_buf_size];
}

bool kbd_hasdata(void)
{
	return kbd_buf_size > 0;
}

int32_t kbd_read(struct file *f, void *top, uint32_t count)
{
	uint8_t *to=top;
	if(count == 0) return 0;
	while(count > kbd_buf_size) __asm__ volatile("sti;hlt;");
	int i=0;
	for(int i=0; i<count; ++i)
	{
		to[i] = kbd_buffer_pop();
	}
	return count;
}

void register_kbd_driver(void)
{
	memset(kbd_buffer, 0, KBD_BUFFER_SIZE);
	kbd_fact.read=&kbd_read;

	register_device(KEYBOARD, &kbd_fact, &kbd_iact);
	struct inode *devfs=vfs_search((struct inode*)root_fs, "/dev");
	devfs_mknod(devfs, "keyboard", KEYBOARD, 0);
}

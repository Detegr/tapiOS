#include <fs/devfs.h>
#include <terminal/vga.h>
#include <util/util.h>
#include <util/scancodes.h>
#include <task/process.h>
#include <task/scheduler.h>
#include <irq/irq.h>

#include "keyboard.h"

#define KBD_BUFFER_SIZE 256

static struct file_actions kbd_fact;
static struct inode_actions kbd_iact;
static uint8_t kbd_buffer[KBD_BUFFER_SIZE];
static volatile uint8_t kbd_buf_size=0;

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

ssize_t kbd_read(struct file *f, void *top, size_t count)
{
	uint8_t *to=top;
	if(count == 0) return 0;
	while(count > kbd_buf_size) sched_yield();
	int i=0;
	for(int i=0; i<count; ++i)
	{
		to[i] = kbd_buffer_pop();
	}
	return count;
}

static void keyboard_isr(void *data, struct registers *regs)
{
	uint8_t status=inb(0x64);
	if(status & 0x1)
	{
		uint8_t scancode=inb(0x60);
		struct process* p=find_active_process();
		if(!p) return; // No active userspace process, nothing to do
		char c=char_for_scancode(scancode);
		if(c==CHAR_UNHANDLED||c==CHAR_UP) return;
		kbd_buffer_push(c);
	}
}

void register_kbd_driver(void)
{
	memset(kbd_buffer, 0, KBD_BUFFER_SIZE);
	kbd_fact.read=&kbd_read;

	register_device(KEYBOARD, &kbd_fact, &kbd_iact);
	struct inode *devfs=vfs_search((struct inode*)root_fs, "/dev");
	devfs_mknod(devfs, "keyboard", KEYBOARD, 0);

	register_isr(1, &keyboard_isr, NULL);
}

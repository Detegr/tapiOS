#ifndef _TAPIOS_PROCESS_H_
#define _TAPIOS_PROCESS_H_

#include <stdint.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <util/util.h>
#include <fs/vfs.h>

#define KERNEL_STACK_SIZE 2048
#define FD_MAX 256

typedef struct process
{
	void *esp;
	void *stack;

	uint32_t pid;

	page_directory* pdir;
	struct process* next;
	vptr_t* esp0;
	bool active;
	bool ready;
	vaddr_t brk;

	uint8_t keyp;
	char keybuf[256];
	char stdoutbuf[256]; // Just a hack before any better implementation

	struct open_files *files_open;
	struct file *fds[FD_MAX];
} process;

#ifndef SHARED_PROCESS_VARIABLES
volatile process* current_process;
volatile process* process_list;
#define SHARED_PROCESS_VARIABLES
#endif

process* find_active_process(void);
int fork(void);
int getpid(void);
int newfd(struct file *f);
vaddr_t init_elf_get_entry_point(uint8_t* elf);
vptr_t *setup_usermode_stack(vaddr_t entry_point, vptr_t *stack_top_ptr);

#endif

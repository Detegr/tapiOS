#ifndef _TAPIOS_PROCESS_H_
#define _TAPIOS_PROCESS_H_

#include <stdint.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <util/util.h>
#include <fs/vfs.h>

#define KERNEL_STACK_SIZE 2048
#define FD_MAX 256

typedef enum process_state
{
	created  = 0x1,
	waiting  = 0x2,
	running  = 0x4,
	blocked  = 0x8,
	finished = 0x10
} process_state;

struct process
{
	vptr_t *user_stack;

	vptr_t *esp0;
	vptr_t *kesp;

	uint32_t pid;

	page_directory* pdir;
	struct process* next;
	vaddr_t brk;

	process_state state;
	bool active;

	uint8_t keyp;
	char keybuf[256];
	char stdoutbuf[256]; // Just a hack before any better implementation

	struct open_files *files_open;
	struct file *fds[FD_MAX];
};

#ifndef SHARED_PROCESS_VARIABLES
volatile struct process *current_process;
volatile struct process *process_list;
#define SHARED_PROCESS_VARIABLES
#endif

struct process* find_active_process(void);
int fork(void);
int getpid(void);
int newfd(struct file *f);
vaddr_t init_elf_get_entry_point(uint8_t* elf);
vptr_t *setup_usermode_stack(vaddr_t entry_point, int argc, char **const argv, vptr_t *stack_top_ptr);

#endif

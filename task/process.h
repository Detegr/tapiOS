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
	uint32_t pid;
	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
	page_directory* pdir;
	struct process* next;
	vptr_t* esp0;
	bool active;
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

void setup_multitasking(void);
process* find_active_process(void);
int fork(void);
int getpid(void);
int newfd(struct file *f);
void switch_to_usermode(vaddr_t entrypoint);

#endif

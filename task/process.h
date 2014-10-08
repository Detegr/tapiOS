#ifndef _TAPIOS_PROCESS_H_
#define _TAPIOS_PROCESS_H_

#include <stdint.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <util/util.h>
#include <fs/vfs.h>
#include <limits.h>

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

struct regs
{
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t ebx;
	uint32_t orig_esp_not_used;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp;
	uint32_t ss;
};

struct process
{
	vptr_t *user_stack;

	vptr_t *esp0;
	vptr_t *kesp;

	pid_t pid;

	page_directory* pdir;
	struct process* next;
	vaddr_t brk;

	process_state state;
	bool active;

	uint8_t keyp;

	//struct open_files *files_open;
	struct file *fds[FD_MAX];

	vptr_t *program;
	char cwd[PATH_MAX];

	char **envp;
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
vptr_t *setup_usermode_stack(vaddr_t entry_point, int argc, char **const argv, char **const envp, vptr_t *stack_top_ptr);
vptr_t *setup_kernel_stack(vaddr_t entry_point, vptr_t *stack_top_ptr);

/* Gets dirname from executable path and assigns it to cwd of process p */
void setcwd_dirname(struct process *p, const char *executable);
void setcwd(struct process *p, const char *path);

#endif

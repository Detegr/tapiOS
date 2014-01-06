#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

#include "sys/dirent.h"

#define EXIT 1
#define WRITE 2
#define READ 3
#define SBRK 4
#define OPEN 5
#define OPENDIR 6
#define READDIR 7
#define FORK 8
#define WAIT 9
#define EXEC 10

#define SYSCALL0(n) \
	__asm__ volatile("int $0x80;" :: "a"(n));

#define SYSCALL1(n,arg1) \
	int ret; \
	__asm__ volatile("int $0x80;" : "=a"(ret) : "0"(n), "b"(arg1)); \
	return ret;

#define SYSCALL2(n,arg1,arg2) \
	int ret; \
	__asm__ volatile("int $0x80;" : "=a"(ret) : "0"(n), "b"(arg1), "c"(arg2)); \
	return ret;

#define SYSCALL3(n,arg1,arg2,arg3) \
	int ret; \
	__asm__ volatile("int $0x80;" : "=a"(ret) : "0"(n), "b"(arg1), "c"(arg2), "d"(arg3)); \
	return (void*)ret;

// Where should the definition of these go?
typedef struct DIR
{
	int dir_fd;
} DIR;

/* pointer to array of char * strings that define the current environment variables */
char **environ;

void _exit() {}
int close(int file) {return -1;}
int fstat(int file, struct stat *st)
{
	return -1;
}
int getpid();
int isatty(int file)
{
	if(file<3) return 1;
	else return 0;
}
int kill(int pid, int sig);
int link(char *old, char *new);
int lseek(int file, int ptr, int dir)
{
	return -1;
}
int open(const char *name, int flags, ...)
{
	SYSCALL2(OPEN, name, flags);
}
int read(int file, char *ptr, int len)
{
	SYSCALL3(READ, file, ptr, len);
}
caddr_t sbrk(int incr)
{
	SYSCALL1(SBRK, incr);
}
int stat(const char *file, struct stat *st);
clock_t times(struct tms *buf);
int unlink(char *name);
int wait(int *status);
int write(int file, char *ptr, int len)
{
	SYSCALL3(WRITE, file, ptr, len);
}

DIR *opendir(const char* name)
{
	SYSCALL1(OPENDIR, name);
}

struct dirent *readdir(DIR *dirp)
{
	static struct dirent de;
	int ret;
	__asm__ volatile("int $0x80;" : "=a"(ret) : "0"(READDIR), "b"(dirp), "c"(&de));
	if(ret==0) return &de;
	else return NULL;
}

int fork(void)
{
	SYSCALL0(FORK);
}

int wait(int *status)
{
	return -1;
}

int execve(const char *path, char **const argv, char **const envp)
{
	SYSCALL3(EXEC, path, argv, envp);
}

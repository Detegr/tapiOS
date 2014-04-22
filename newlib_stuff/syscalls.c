#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <signal.h>

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
#define CLOSEDIR 11
#define GETPID 12

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
	return ret;

// Where should the definition of these go?
typedef struct DIR
{
	int dir_fd;
} DIR;

/* pointer to array of char * strings that define the current environment variables */
char **environ;

void _exit()
{
	SYSCALL0(EXIT);
}
int close(int file) {return -1;}
int fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}
pid_t getpid(void)
{
	SYSCALL0(GETPID);
}
int getppid()
{
	return -1;
}
int isatty(int file)
{
	if(file<3) return 1;
	else return 0;
}
int kill(int pid, int sig)
{
	return -1;
}
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
int stat(const char *file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}
clock_t times(struct tms *buf);
int unlink(char *name);
int write(int file, char *ptr, int len)
{
	SYSCALL3(WRITE, file, ptr, len);
}

DIR *opendir(const char* name)
{
	SYSCALL1(OPENDIR, name);
}

int closedir(DIR *dirp)
{
	SYSCALL1(CLOSEDIR, dirp);
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
	SYSCALL1(WAIT, status);
}

pid_t wait3(int *status, int options, struct rusage *rusage)
{
	errno=ECHILD;
	return -1;
}

int execve(const char *path, char **const argv, char **const envp)
{
	SYSCALL3(EXEC, path, argv, envp);
}

int dup2(int oldfd, int newfd)
{
	return -1;
}

int pipe(int pipefd[2])
{
	return -1;
}

mode_t umask(mode_t mask)
{
	return 000;
}

int chdir(const char *path)
{
	return -1;
}

int fcntl(int fd, int cmd, ...)
{
	return -1;
}

uid_t geteuid(void)
{
	return -1;
}

gid_t getgid(void)
{
	return -1;
}

gid_t getegid(void)
{
	return -1;
}

char *getcwd(char *buf, size_t size)
{
	if(size >= 2)
	{
		memcpy(buf, "/", 2);
		return buf;
	}
	return NULL;
}

int lstat(const char *path, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

pid_t tcgetpgrp(int fd)
{
	return -1;
}

int tcsetpgrp(int fd, pid_t grp)
{
	return -1;
}

pid_t getpgrp(void)
{
	return -1;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	return -1;
}

int setpgid(pid_t pid, pid_t pgid)
{
	return -1;
}

uid_t getuid(void)
{
	return -1;
}

int getgroups(size_t gidsetsize, const gid_t *list)
{
	return -1;
}

int sigsuspend(const sigset_t *mask)
{
	return -1;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	return -1;
}

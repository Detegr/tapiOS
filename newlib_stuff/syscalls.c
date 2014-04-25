#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "sys/dirent.h"

#define EXIT 1
#define WRITE 2
#define READ 3
#define SBRK 4
#define OPEN 5
#define OPENDIR 6
#define READDIR 7
#define FORK 8
#define WAITPID 9
#define EXEC 10
#define CLOSEDIR 11
#define GETPID 12
#define FSTAT 13
#define GETCWD 14
#define CHDIR 15
#define CLOSE 16

#define SYSCALL0(n) \
	int ret; \
	__asm__ volatile("int $0x80;" : "=a"(ret) : "0"(n)); \
	if(ret<0) { errno=-ret; return -1; } \
	return ret;

#define SYSCALL1(n,arg1) \
	int ret; \
	__asm__ volatile("int $0x80;" : "=a"(ret) : "0"(n), "b"(arg1)); \
	if(ret<0) { errno=-ret; return -1; } \
	return ret;

#define SYSCALL2(n,arg1,arg2) \
	int ret; \
	__asm__ volatile("int $0x80;" : "=a"(ret) : "0"(n), "b"(arg1), "c"(arg2)); \
	if(ret<0) { errno=-ret; return -1; } \
	return ret;

#define SYSCALL3(n,arg1,arg2,arg3) \
	int ret; \
	__asm__ volatile("int $0x80;" : "=a"(ret) : "0"(n), "b"(arg1), "c"(arg2), "d"(arg3)); \
	if(ret<0) { errno=-ret; return -1; } \
	return ret;

/* pointer to array of char * strings that define the current environment variables */
char **environ;

struct DIR
{
	int dir_fd;
};

void _exit()
{
	SYSCALL0(EXIT);
}
int close(int fd)
{
	SYSCALL1(CLOSE, fd);
}
int fstat(int fd, struct stat *st)
{
	SYSCALL2(FSTAT, fd, st);
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
int stat(const char *path, struct stat *st)
{
	int fd=open(path, O_RDONLY);
	if(fd<0) return -1;

	int ret = fstat(fd, st);
	close(fd);
	if(ret<0) return -1;

	errno=0;
	return ret;
}
clock_t times(struct tms *buf);
int unlink(char *name);
int write(int file, char *ptr, int len)
{
	SYSCALL3(WRITE, file, ptr, len);
}

DIR *opendir(const char* name)
{
	int fd=open(name, O_RDONLY);
	if(fd<0) return NULL;
	struct stat st;
	if((fstat(fd, &st)) < 0) return NULL;
	if(!S_ISDIR(st.st_mode))
	{
		errno=-ENOTDIR;
		return NULL;
	}
	DIR *ret=malloc(sizeof(DIR));
	ret->dir_fd=fd;
	return ret;
}

int closedir(DIR *dirp)
{
	int ret=close(dirp->dir_fd);
	if(ret<0) return -1;
	free(dirp);
}

struct dirent *readdir(DIR *dirp)
{
	static struct dirent de;
	int ret;
	__asm__ volatile("int $0x80;" : "=a"(ret) : "0"(READDIR), "b"(dirp->dir_fd), "c"(&de));
	if(ret==0) return &de;
	else return NULL;
}

int fork(void)
{
	SYSCALL0(FORK);
}

pid_t wait(int *status)
{
	SYSCALL3(WAITPID, -1, status, 0);
}

pid_t wait3(int *status, int options, struct rusage *rusage)
{
	SYSCALL3(WAITPID, -1, status, options);
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
	SYSCALL1(CHDIR, path);
}

int fcntl(int fd, int cmd, ...)
{
	return -1;
}

uid_t geteuid(void)
{
	return 0;
}

gid_t getgid(void)
{
	return -1;
}

gid_t getegid(void)
{
	return -1;
}

static int _getcwd(char *buf, size_t size)
{
	SYSCALL2(GETCWD, buf, size);
}

char *getcwd(char *buf, size_t size)
{
	_getcwd(buf, size);
	return buf;
}

int lstat(const char *path, struct stat *st)
{
	return stat(path, st);
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
	return 0;
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

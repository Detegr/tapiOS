#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

int command(const char *cmd, const char *cmd_space, const char *cmp)
{
	return (strcmp(cmp, cmd)==0 || strncmp(cmp, cmd_space, strlen(cmd_space)) == 0);
}

int main(int argc, char **argv)
{
	printf("tapiShell v0.0.1\n");
	char buf[1024];
	char cwd[1024];
	memset(buf, 0, 1024);
	memset(cwd, 0, 1024);
	while(1)
	{
		printf("tapiShell :: %s # ", cwd[0] ? cwd : "/");
		fflush(stdout);
		fgets(buf, 1024, stdin);
		buf[strlen(buf)-1]=0;

		if(command("cd","cd ",buf))
		{
			strtok(buf, " ");
			char *arg=strtok(NULL, " ");
			if(arg)
			{
				char to[1024];
				memset(to, 0, 1024);
				stpcpy(stpcpy(stpcpy(to, cwd), "/"), arg);
				DIR *d=opendir(to);
				if(d) strcpy(cwd, to);
				else
				{
					printf("tapiShell :: No such directory: '%s'\n", arg);
				}
			}
		}
		else if(command("cat", "cat ", buf))
		{
			strtok(buf, " ");
			char *arg=strtok(NULL, " ");
			if(arg)
			{
				char to[1024];
				memset(to, 0, 1024);
				stpcpy(stpcpy(stpcpy(to, cwd), "/"), arg);
				int fd=open(to, O_RDONLY);
				if(fd>0)
				{
					char filebuf[1024];
					read(fd, filebuf, 1024);
					printf("%s", filebuf);
				}
			}
		}
		else if(strlen(buf)>0)
		{
			int pid=fork();
			if(!pid)
			{
				char *argv[3];
				char cmd[1024];
				memset(cmd, 0, 1024);
				stpcpy(stpcpy(cmd, "/bin/"), buf);
				argv[0]=cmd;
				argv[1]=cwd[0] ? cwd : "/";
				argv[2]=NULL;
				execve(cmd, argv, NULL);
				printf("tapiShell :: Command not found: '%s'\n", buf);
				__asm__ volatile("mov $1, %eax; int $0x80;");
			}
			wait(NULL);
		}
	}
	return 0;
}

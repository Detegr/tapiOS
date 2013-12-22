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

int main()
{
	char buf[1024];
	char cwd[1024];
	while(1)
	{
		printf("tapiShell :: %s # ", cwd[0] ? cwd : "/");
		fflush(stdout);
		fgets(buf, 1024, stdin);
		buf[strlen(buf)-1]=0;
		if(command("ls", "ls ", buf))
		{
			DIR *d=opendir(cwd[0] ? cwd : "/");
			struct dirent *dep;
			while((dep=readdir(d)))
			{
				printf("%s\n", dep->d_name);
			}
		}
		else if(command("cd","cd ",buf))
		{
			strtok(buf, " ");
			char *arg=strtok(NULL, " ");
			if(arg)
			{
				char to[1024];
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
		else
		{
			printf("tapiShell :: Command not found: '%s'\n", buf);
		}
	}
	return 0;
}

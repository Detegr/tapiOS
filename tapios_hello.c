#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

int main()
{
	char buf[1024];
	char cwd[1024];
	cwd[0]='/';
	while(1)
	{
		printf("tapiShell :: %s # ", cwd);
		fflush(stdout);
		scanf("%s", buf);
		if(strcmp(buf, "ls") == 0)
		{
			DIR* d=opendir(cwd);
			struct dirent *dep;
			while((dep=readdir(d)))
			{
				printf("%s\n", dep->d_name);
			}
		}
		else
		{
			printf("tapiShell :: Command not found: '%s'\n", buf);
		}
		
	}
	/*
	int fd=open("/dir/file.txt", 0, 0);
	if(fd>0)
	{
		char buf[1024];
		int r=read(fd, buf, 1024);
		printf("Read: %d\n", r);
		printf("%s", buf);
	}
	else
	{
		printf("Failed to open file.\n");
	}
	*/
	return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

extern unsigned char __bss_start;
extern unsigned char __bss_end;

int main()
{
	printf("Hello tapiOS!\n");
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
	DIR* d=opendir("/dir");
	return 0;
}

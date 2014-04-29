#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>

int main()
{
	open("/dev/keyboard", O_RDONLY);
	open("/dev/terminal", O_WRONLY);
	open("/dev/terminal", O_WRONLY);

	printf("tapiOS init\n");
	int pid=fork();
	if(pid == 0)
	{
		char *argv[2];
		char *cmd="/bin/dash";
		argv[0]=cmd;
		argv[1]=NULL;
		execve(cmd, argv, NULL);
	}
	else
	{
		wait(NULL);
	}
	return 0;
}

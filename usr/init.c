#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main()
{
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

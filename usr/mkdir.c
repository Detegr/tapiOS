#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

int main(int argc, const char **argv)
{
	if(argc<2)
	{
		fprintf(stderr, "Missing pathname\n");
		return -1;
	}
	if(mkdir(argv[1], 0777) < 0)
	{
		printf("Error %d\n", errno);
		return -1;
	}
	return 0;
}

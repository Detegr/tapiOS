#include <unistd.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
	if(argc<2) return -1;
	int fd=open(argv[1], O_CREAT);
	if(fd>0) close(fd);
	return 0;
}

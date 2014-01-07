#include <dirent.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	if(argc<2) return -1;
	DIR *d=opendir(argv[1] ? argv[1] : "/");
	struct dirent *dep;
	while((dep=readdir(d)))
	{
		printf("%s  ", dep->d_name);
	}
	printf("\n");
	return 0;
}

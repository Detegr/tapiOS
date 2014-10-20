#include <dirent.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if(argc<1) return -1;
	char cwd[PATH_MAX];
	getcwd(cwd, PATH_MAX);
	DIR *d=opendir(cwd);
	struct dirent *dep;
	while((dep=readdir(d)))
	{
		printf("%s  ", dep->d_name);
	}
	closedir(d);
	printf("\n");
	return 0;
}

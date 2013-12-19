struct DIR;
typedef struct DIR DIR;

struct dirent
{
	int d_ino;
	char d_name[256];
};

DIR *opendir(const char* name);
struct dirent *readdir(DIR *dirp);

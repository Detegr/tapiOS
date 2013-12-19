struct DIR;
typedef struct DIR DIR;

DIR *opendir(const char* name);
struct dirent *readdir(DIR *dirp);

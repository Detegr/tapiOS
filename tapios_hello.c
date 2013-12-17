#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
	char str[32];
	char str2[32];
	char str3[32];
	printf("Hello\n");
	scanf("%s %s %s", str, str2, str3);
	printf("First: %s\nSecond: %s\nThird: %s\n", str, str2, str3);
	return 0;
}

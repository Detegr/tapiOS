#include "random.h"
#define RAND_MAX 2147483647U

static uint32_t next=0x71B105;

int rand(void)
{
	next = next * 1103515245 + 12345;
	return (uint32_t)(next / 65536) % (RAND_MAX+1);
}

void srand(uint32_t seed)
{
	next = seed;
}

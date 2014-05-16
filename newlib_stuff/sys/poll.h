#include <stdint.h>

#define POLLIN            (1<<0)
#define POLLWRNORM POLLIN
#define POLLRDNORM        (1<<1)
#define POLLRDBAND        (1<<2)
#define POLLPRI           (1<<3)
#define POLLOUT           (1<<4)
#define POLLWRBAND        (1<<5)
#define POLLERR           (1<<6)
#define POLLHUP           (1<<7)
#define POLLNVAL          (1<<8)

struct pollfd
{
	int fd;
	uint16_t events;
	uint16_t revents;
};

typedef long nfds_t;

int poll(struct pollfd *fds, nfds_t nfds, int timeout);

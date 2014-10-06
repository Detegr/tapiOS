#ifndef _TAPIOS_SOCKET_H_
#define _TAPIOS_SOCKET_H_

#include <util/list.h>
#include <util/util.h>

#define CONNECTING 1
#define CONNECTED 2

struct socket
{
	// NOTE: Keep these two in the beginning to allow casting to (struct file*) work
	uint32_t refcount;
	struct inode *inode;
	// End of note
	uint16_t port;
	uint8_t state;

	struct list list;
};

volatile struct socket *open_sockets;

#endif

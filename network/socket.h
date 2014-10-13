#ifndef _TAPIOS_SOCKET_H_
#define _TAPIOS_SOCKET_H_

#include <util/list.h>
#include <util/util.h>
#include <sys/socket.h>

#define CONNECTING 1
#define CONNECTED 2
#define DISCONNECTING 3
#define DISCONNECTED 4

struct socket
{
	// NOTE: Keep these two in the beginning to allow casting to (struct file*) work
	uint32_t refcount;
	struct inode *inode;
	// End of note
	struct sockaddr_in saddr;
	uint16_t src_port;
	uint32_t seq_no;
	uint32_t ack_no;
	uint8_t state;
	uint8_t dst_mac[6];

	struct list list;
};

volatile struct socket *open_sockets;

#endif

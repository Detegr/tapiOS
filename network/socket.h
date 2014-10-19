#ifndef _TAPIOS_SOCKET_H_
#define _TAPIOS_SOCKET_H_

#include <util/list.h>
#include <util/util.h>
#include <sys/socket.h>
#include <fs/vfs.h>

#define CONNECTING 1
#define CONNECTED 2
#define DISCONNECTING 3
#define DISCONNECTED 4

struct socket
{
	// NOTE: Keep these two in the beginning to allow casting to (struct file*) work
	uint32_t refcount;
	struct inode *inode;
	uint32_t pos;
	// End of note
	struct sockaddr_in saddr;
	uint16_t src_port;
	uint32_t seq_no;
	uint32_t ack_no;
	uint8_t state;
	uint8_t dst_mac[6];

	uint8_t *rcvbuf;

	struct list list;
};

volatile struct socket *open_sockets;
void socket_free(struct socket *s);
ssize_t socket_read(struct file *f, void *to, size_t count);
ssize_t socket_write(struct file *f, const void *data, size_t count);
int32_t socket_close(struct file *f);
int32_t socket_poll(struct file *f, uint16_t events, uint16_t *revents);

#endif

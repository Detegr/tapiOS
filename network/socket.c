#include "socket.h"
#include "tcp.h"
#include <terminal/vga.h>
#include <mem/kmalloc.h>
#include <sys/poll.h>
#include <task/process.h>

void socket_free(struct socket *s)
{
	s->refcount--;
	kfree(s->rcvbuf);
	if(s->refcount) kprintf("Tried to free a socket that has refcount %d!\n", s->refcount);
	else kfree(s);
}

ssize_t socket_write(struct file *file, const void *data, size_t count)
{
	struct network_device *dev=file->inode->device;
	struct socket *s=(struct socket*)file;
	uint8_t buf[sizeof(struct tcp_packet) + count];
	struct tcp_packet *p=(struct tcp_packet*)&buf;
	struct tcp_opts opts={
		.ACK=1
	};
	tcp_build_packet(dev, s->dst_mac, &s->saddr, p, &opts, data, count);
	p->tcp_header.port_src=s->src_port;
	p->tcp_header.seq_no=s->seq_no;
	p->tcp_header.ack_no=s->ack_no;
	p->tcp_header.checksum=tcp_checksum(p, count);
	s->seq_no=p->tcp_header.seq_no + htonl(count);
	s->ack_no=p->tcp_header.ack_no;
	return dev->n_act->tx(file, buf, sizeof(buf));
}

ssize_t socket_read(struct file *f, void *to, size_t count)
{
	struct socket *s=(struct socket*)f;
	size_t n=s->pos<count ? s->pos : count;
	if(n==0) return 0;
	memcpy(to, s->rcvbuf, n);
	uint32_t remainder=s->pos-n;
	if(remainder>0)
	{
		memmove(s->rcvbuf, s->rcvbuf+remainder, remainder);
	}
	s->pos-=n;
	return n;
}

int32_t socket_close(struct file *f)
{
	struct socket *s=(struct socket*)f;
	if(s->state==CONNECTED)
	{
		struct tcp_opts opts={
			.FIN=1,
			.ACK=1
		};
		s->state=FIN_WAIT1;
		tcp_send_packet(s->inode->device, s, s->seq_no, s->ack_no, &opts);
	}
	return 0;
}

int32_t socket_poll(struct file *f, uint16_t events, uint16_t *revents)
{
	int32_t ret=0;
	if(f && (events & POLLIN))
	{
		struct socket *s=(struct socket *)f;
		if(f->pos>0 || s->state==DISCONNECTED)
		{
			/* Posix specifies that actual data is not needed to be present.
			 * For POLLIN to return true, only non-blocking read is required.
			 */
			*revents |= POLLIN;
			ret=1;
		}
	}
	if(events & POLLNVAL)
	{
		if(!f)
		{
			*revents |= POLLNVAL;
			ret=1;
		}
	}
	return ret;
}

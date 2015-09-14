#ifndef SOCKET_H
#define SOCKET_H

#define AF_INET 1
#define SOCK_STREAM 1

#include <stdint.h>

#define htons(x) ((((x) & 0xFF)) << 8 | (((x) & 0xFF00) >> 8))
#define htonl(x) ((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | (((x) & 0xFF0000) >> 8) | (((x) & 0xFF000000) >> 24))

struct sockaddr
{
	uint16_t sa_family;
	uint8_t sa_data[14];
};
struct sockaddr_in
{
	uint16_t sin_family;
	uint16_t sin_port;
	uint32_t sin_addr;
	uint8_t sin_zero[8];
};

int socket(int,int,int);
int connect(int,const struct sockaddr*,int);

#endif

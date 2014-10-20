/* The most awful IRC "client" in the universe, enjoy :) */

#include <sys/poll.h>
#include <sys/socket.h>
#ifndef TAPIOS
#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#ifdef TAPIOS
#include <ncurses/ncurses.h>
#else
#include <ncurses.h>
#endif

#define BUFSIZE 2048

static const char *rn="\r\n";
static WINDOW *win;

void tapiirc_send(int fd, const char *msg, size_t count)
{
	int len=count+2;
	char buf[len];
	strcpy(buf, msg);
	memcpy(buf+len-2, rn, 2);
	write(fd, buf, len);
}

void handle_pingpong(int fd, char *str)
{
	str[1]='O';
	tapiirc_send(fd, str, strlen(str));
}

void handle_message(int fd, char *buf)
{
	static uint32_t text_cursor_pos=0;
	move(text_cursor_pos,0);
	char *str=strtok(buf, "\r\n");
	do {
		if(strncmp(str, "PING", 4) == 0) handle_pingpong(fd, str);
		clrtoeol();
		addstr(str);
		move(++text_cursor_pos, 1);
		str=strtok(NULL, "\r\n");
	} while(str);
	text_cursor_pos=getcury(win);
	if(text_cursor_pos>=LINES-1) text_cursor_pos=0;
	move(LINES-1, 0);
}

int main(int argc, char **argv)
{
	win=initscr();
	cbreak();
	noecho();
	mvwprintw(win, 0, 0, "tapiIRC. Please press a key to connect.");
	mvwprintw(win, LINES-1, 0, "Waiting for keypress...");
	move(LINES,COLS);
	getch();
	move(LINES-1, 0);
	clrtoeol();
	move(0,0);
	clrtoeol();

	int fd=socket(AF_INET,SOCK_STREAM,0);
	if(fd<0)
	{
		//printf("Could not create socket\n");
		return 1;
	}
	struct sockaddr_in addr;
#ifdef TAPIOS
	uint32_t ia=htonl(0x14000001);
#else
	struct in_addr ia;
	inet_pton(AF_INET, "127.0.0.1", &ia);
#endif
	addr.sin_family=AF_INET;
	addr.sin_port=htons(6667);
	memset(addr.sin_zero, 0, 8);
	addr.sin_addr = ia;
	//printf("tapiIRC connecting...");
	if(connect(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)))
	{
		printf("Connection refused\n");
		return 1;
	}
	tapiirc_send(fd, "NICK tapiOS", 11);
	tapiirc_send(fd, "USER tapiOS 0 * :tapiOS user", 28);

	struct pollfd pfd={
		.fd=fd,
		.events=POLLIN,
		.revents=0
	};
	struct pollfd kbd={
		.fd=STDIN_FILENO,
		.events=POLLIN,
		.revents=0
	};

	char kbdbuf[BUFSIZE];
	char buf[BUFSIZE];
	int kbdbufptr=0;
	memset(kbdbuf, 0, BUFSIZE);
	while(1)
	{
		wrefresh(win);
		int data=poll(&pfd, 1, 1);
		if(data>0)
		{
			memset(buf, 0, BUFSIZE);
			if(read(fd, buf, BUFSIZE) <= 0) goto quit;
			else handle_message(fd, buf);
		}
		do {
			data=poll(&kbd, 1, 1);
			if(data>0)
			{
				int c=fgetc(stdin);
				if(c=='\n' || c=='\r')
				{
					tapiirc_send(fd, kbdbuf, kbdbufptr);
					kbdbufptr=0;
					memset(kbdbuf, 0, BUFSIZE);
					move(LINES-1, 0);
					clrtoeol();
				}
				else
				{
					kbdbuf[kbdbufptr++]=(char)c;
					mvaddstr(LINES-1, 0, kbdbuf);
				}
			}
		} while(data>0);
	}
quit:
	close(fd);
	endwin();
	return 0;
}

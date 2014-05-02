#ifndef TERMIOS_H
#define TERMIOS_H

#include <stdint.h>

typedef uint32_t tcflag_t;
typedef uint32_t cc_t;
typedef uint32_t speed_t;

// c_lflag
#define ECHO (1<<0)
#define ECHONL (1<<1)
#define ECHONL (1<<1)
#define ICANON (1<<2)
#define ISIG (1<<3)
#define IEXTEN (1<<4)
#define NOFLSH (1<<5)
#define ECHOE (1<<6)
#define ECHOK (1<<7)

// c_iflag
#define INGBRK (1<<0)
#define BRKINT (1<<1)
#define PARMRK (1<<2)
#define ISTRIP (1<<3)
#define INLCR (1<<4)
#define IGNCR (1<<5)
#define ICRNL (1<<6)
#define IXON (1<<7)
#define IGNBRK (1<<8)
#define IGNPAR (1<<9)
#define INPCK (1<<10)
#define IXOFF (1<<11)
#define ALLIN (1<<12)
#define ALLOUT (1<<13)
#define ALLCTRL (1<<14)

// c_oflag
#define OPOST (1<<0)

// c_cflag
#define CSIZE (1<<0)
#define PARENB (1<<1)
#define CS8 (1<<2)
#define CLOCAL (1<<3)
#define CREAD (1<<4)
#define CSTOPB (1<<5)
#define HUPCL (1<<6)
#define PARODD (1<<7)

#define B0       0
#define B110     3
#define B1200    9
#define B134     4
#define B150     5
#define B1800    10
#define B19200   14
#define B200     6
#define B2400    11
#define B300     7
#define B38400   15
#define B4800    12
#define B50      1
#define B600     8
#define B75      2
#define B9600    13

#define TCIFLUSH 0
#define VERASE 1
#define VKILL 2
#define VMIN 3
#define VTIME 4
#define VEOF 5
#define VINTR 6
#define VQUIT 6
#define NCCS 7

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

struct termios
{
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_cc[NCCS];
};

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcflush(int fd, int queue_selector);
speed_t cfgetospeed(const struct termios *termios_p);

#endif

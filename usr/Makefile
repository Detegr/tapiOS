CC=/home/antti/Code/tapiOS/cross/bin/i586-pc-tapios-gcc
OBJECTS=ls init cat dash-0.5.7/build/src/dash touch mkdir
CFLAGS=-Wall -Wextra -m32 -g3 -std=c99 -masm=intel -isystem../cross/i586-pc-tapios/include -Ttapios.ld
LFLAGS=-melf_i386 -Lcross/i586-pc-tapios/lib -lc

all: $(OBJECTS) copy unmount

unmount:
	umount ../tapios/mnt

copy:
	mount ../tapios/initrd
	cp $(OBJECTS) ../tapios/mnt/bin

dash-0.5.7/build/src/dash:
	make -C dash-0.5.7/build

% : %.c %.h
	$(CC) $(CFLAGS) $< -o $@ -g3

clean:
	-rm $(OBJECTS)

.PHONY: all clean unmount copy

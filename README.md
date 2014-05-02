<p align="center">
  <img src="https://raw.githubusercontent.com/Detegr/tapiOS/master/tapios_logo.png?raw=true" alt="tapiOS logo"/>
</p>
## tapiOS
tapiOS is my hobby operating system. It is a monolithic kernel (unix clone) and is developed for learning purposes only.

## Compiling
Just don't. Makefile has hardcoded paths to cross compiler etc. and all the userspace stuff must be compiled separately and must be copied to a file with ext2 filesystem inside. That said, I don't think anyone will be crazy enough to be interested in compiling this project. If you still want to do that, please contact me for further details.

## Build tricks
Various flags and tricks for building things to tapiOS documented here.

- Configure: `LDFLAGS=usr/tapios.ld`
- newlib: `CFLAGS=-D_I386MACH_ALLOW_HW_INTERRUPTS --enable-newlib-nano-malloc`
- dash: `#undef _GNU_SOURCE` to config.h
- ncurses: `--with-build-cc=gcc --without-cxx`

global _setgdt
global _setidt
global _outb
global _outw
global _io_wait
global _idle
global _panic
global _inb
global _get_eip
global _return_to_userspace

extern gdtptr
extern idtptr
extern test
extern panic

section .text

_outb:
	mov al, [esp+8]
	mov dx, [esp+4]
	out dx, al
	ret

_outw:
	push ebp
	mov ebp, esp
	mov ax, [esp+8]
	mov dx, [esp+4]
	out dx, ax
	pop ebp
	ret

_inb:
	mov edx, [esp+4]
	in al, dx
	ret

_io_wait:
	mov eax, 0x0
	mov dx, 0x3A4 ; 0x3A4 should be unused
	out dx, eax
	ret

_idle:
	hlt
	ret

_panic:
	cli
	hlt

_get_eip:
	pop eax
	jmp eax

_return_to_userspace:
	pop gs
	pop fs
	pop es
	pop ds
	popad
	iret

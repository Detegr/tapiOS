global _setgdt
global _setidt
global _outb
global _io_wait
global _idle
global _panic
global _inb

extern gdtptr
extern idtptr
extern test
extern panic

section .text

_setgdt:
	lgdt [gdtptr]
	jmp 0x08:flushgdt ; 0x08 is our new code selector in gdt
flushgdt:
	mov ax, 0x10 ; 0x10 is the new data selector
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	mov ss,ax
	ret

_setidt:
	lidt [idtptr]
	ret

_outb:
	mov al, [esp+8]
	mov dx, [esp+4]
	out dx, al
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
	call panic
	cli
	hlt

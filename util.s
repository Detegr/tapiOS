global _setgdt
global _setidt
global _outb
global _io_wait

extern gdtptr
extern idtptr

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
	mov eax, [esp+8]
	mov dx, [esp+4]
	out dx, eax
	ret

_io_wait:
	mov eax, 0x0
	out 0x3A4, eax ; 0x3A4 should be unused
	ret

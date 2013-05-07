global setgdt
global setidt

extern gdtptr
extern idtptr

section .text

setgdt:
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

setidt:
	lidt [idtptr]
	ret

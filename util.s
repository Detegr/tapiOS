global setgdt
extern gdtptr

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

;setidt:
;	mov eax, [esp+4]
;	mov [idt+2], eax
;	mov ax, [esp+8]
;	mov [idt], ax
;	lidt [idt]

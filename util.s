global setgdt
global setidt

section .text

gdt dw 0
	dd 0

idt dw 0
	dd 0

setgdt:
	mov eax, [esp+4]
	mov [gdt+2], eax
	mov ax, [esp+8]
	mov [gdt], ax
	lgdt [gdt]

setidt:
	mov eax, [esp+4]
	mov [idt+2], eax
	mov ax, [esp+8]
	mov [idt], ax
	lidt [idt]

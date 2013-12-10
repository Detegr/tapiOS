section .data:
	str: db "Hello from external process! It's alive!",10
	strlen equ $-str
section .bss:
section .text:
global _start

_start:
	mov eax, 0x2
	mov ebx, str
	mov ecx, strlen
	int 0x80
	mov eax, 0x1
	int 0x80

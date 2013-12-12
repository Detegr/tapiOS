section .bss
	buf: resb 32
section .data
	str: db "Please write something:",10
	strlen equ $-str
	str2: db "Thank you, you wrote: "
	strlen2 equ $-str2
	linebreak: db 10
section .text
global _start

_start:
	mov eax, 0x2
	mov ebx, 0x0 ; Not used
	mov ecx, str
	mov edx, strlen
	int 0x80
loop:
	mov eax, 0x3
	mov ebx, 0x0 ; Not used
	mov ecx, buf
	mov edx, 32
	int 0x80
	mov edi, eax
	mov eax, 0x2
	mov ebx, 0x0 ; Not used
	mov ecx, str2
	mov edx, strlen2
	int 0x80
	mov edx, edi
	mov eax, 0x2
	mov ecx, buf
	int 0x80
	mov eax, 0x2
	mov ecx, linebreak
	mov edx, 1
	int 0x80
	jmp loop
exit:
	mov eax, 0x1
	mov ebx, 0x0
	int 0x80

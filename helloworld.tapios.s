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
	mov ebx, str
	mov ecx, strlen
	int 0x80
loop:
	mov eax, 0x3
	mov ebx, buf
	mov ecx, 32
	int 0x80
	mov edi, eax
	mov eax, 0x2
	mov ebx, str2
	mov ecx, strlen2
	int 0x80
	mov ecx, edi
	mov eax, 0x2
	mov ebx, buf
	int 0x80
	mov eax, 0x2
	mov ebx, linebreak
	mov ecx, 1
	int 0x80
	jmp loop
exit:
	mov eax, 0x1
	int 0x80

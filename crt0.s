global _start
extern main
extern __bss_start
extern __bss_end

_start:
	; Zero out bss
	mov edi, __bss_start
	mov ecx, __bss_end
	sub ecx, edi
	xor al, al
	rep stosb

	call main
	mov ebx, eax
	mov eax, 0x1
	int 0x80
hltloop:
	hlt
	jmp hltloop

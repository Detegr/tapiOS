global _start
extern main
_start:
	call main
	mov ebx, eax
	mov eax, 0x1
	int 0x80
hltloop:
	hlt
	jmp hltloop

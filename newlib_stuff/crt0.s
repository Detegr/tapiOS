.globl _start
.extern main
.extern __bss_start
.extern __bss_end
.extern environ

_start:
	popl %ecx # DS
	movl $__bss_start, %edi
	movl $__bss_end, %ecx
	sub %edi, %ecx
	xor %al, %al
	rep stosb

	popl %ecx # envp
	testl %ecx, %ecx
	je skipenv # envp NULL, skip copying the pointer
	movl %ecx, environ
skipenv:
	call main
	movl %eax, %ebx
	movl $0x1, %eax
	int $0x80
hltloop:
	hlt
	jmp hltloop

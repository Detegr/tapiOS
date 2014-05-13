[section .setup]

global _setup_page_table
global _map_page
global _enable_paging
global _invalidate_page_table

KERNEL_VMA equ 0xC0000000
FLAG_PRESENT equ 0x1
FLAG_READWRITE equ 0x2
FLAG_USERMODE equ 0x4

INITIAL_KERNEL_CODE_FLAGS equ FLAG_PRESENT|FLAG_READWRITE

_setup_page_table:
	push eax
	mov eax, ebx
	call _calculate_directory_position    ; to edx
	pop eax
	or ecx, INITIAL_KERNEL_CODE_FLAGS
	mov [eax + edx * 4], ecx
	ret

_map_page:
	push ebp
	mov ebp, esp

	sub esp, 4
	mov [esp-4], eax                      ; Page directory location
	mov esi, edx                          ; Physical address
	or esi, INITIAL_KERNEL_CODE_FLAGS     ; Set correct flags

	shr ebx, 20                           ; Directory position
	add ebx, eax                          ; Shift only 20 bits and add directory location
	mov eax, ecx                          ; Original count

	.fill:
		mov edi, eax                      ; Original count
		sub edi, ecx                      ; pages mapped
		shl edi, 2                        ; Multiply by 4

		mov edx, [ebx]                    ; get old entry
		and edx, FLAG_PRESENT
		test edx, edx                     ; if not present
		je __panic                        ; jump to panic

		mov edx, [ebx]                    ; Fetch page table from the correct directory index
		and edx, 0xFFFFF000               ; Zero out unnecessary bits
		add edx, edi                      ; page table + mapped bytes

		mov [edx], esi                    ; set physical address to correct page table
		add esi, 0x1000                   ; advance to next page

		loop .fill

	mov eax, [esp-4]
	add esp, 4
	mov esp, ebp
	pop ebp
	ret

__panic:
	hlt
	jmp __panic

_calculate_directory_position:
	mov edx, eax
	shr edx, 22                           ; Index in page directory
	ret

_enable_paging:
	mov ebx, eax
	mov cr3, eax ; Put the address of page directory to cr3
	; Set paging bit on in cr0
	mov eax, cr0
	or eax, 0x80000000 ; Set paging bit
	mov cr0, eax
	add esp, 0xC0000000
	mov ebp, esp

	ret

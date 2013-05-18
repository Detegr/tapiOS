[section .setup]

global _setup_page_table
global _map_page
global _enable_paging
global _invalidate_page_table

KERNEL_VMA equ 0xC0000000
FLAG_PRESENT equ 0x1
FLAG_READWRITE equ 0x2

_setup_page_table:
	push eax
	mov eax, ebx
	call _calculate_directory_position    ; to edx
	pop eax
	or ecx, FLAG_PRESENT|FLAG_READWRITE
	mov [eax + edx * 4], ecx
	ret

_map_page:
	push ebp
	mov ebp, esp

	sub esp, 30
	mov [esp-30], DWORD 0                 ; Directory index
	mov [esp-24], eax                     ; Directory
	mov [esp-20], ebx                     ; Virtual address
	mov [esp-16], ecx                     ; Count
	mov [esp-12], ecx                     ; Original count
	mov [esp-8] , DWORD 0                 ; Mapped bytes
	mov [esp-4] , edx                     ; Physical address

	.fill:
		mov [esp-16], ecx                 ; Count

		mov edx, ebx
		shr edx, 22                       ; Directory position
		mov [esp-30], edx                 ; Index

		mov eax, [esp-12]                 ; Original count
		sub eax, ecx                      ; pages mapped
		mov ecx, 4
		mul ecx
		mov [esp-8], eax                  ; mapped bytes TODO: What if result is in edx too?
		mov edx, [esp-30]                 ; Index
		mov eax, [esp-24]                 ; Directory
		mov ebx, [esp-8]                  ; Mapped bytes

		mov ecx, [eax + edx * 4]          ; get old entry
		and ecx, FLAG_PRESENT
		cmp ecx, 0x0                      ; if not present
		mov ecx, [esp-16]                 ; Store count for debugging
		je __panic                        ; jump to panic

		mov ecx, [esp-16]                 ; CountCount

		mov eax, [eax + edx * 4]          ; Fetch page table from the correct directory index
		and eax, 0xFFFFF000               ; Zero out unnecessary bytes
		add eax, ebx                      ; page table + mapped bytes
		mov ebx, [esp-4]  	              ; physical address

		mov [eax], ebx                    ; set physical address to correct page table
		or DWORD [eax], FLAG_PRESENT|FLAG_READWRITE

		add ebx, 0x1000                   ; advance to next page
		mov [esp-4], ebx                  ; Physical address

		mov ebx, [esp-20]				  ; advance virtual address too
		add ebx, 0x1000
		mov [esp-20], ebx 

		loop .fill

	mov eax, [esp-24]                     ; Directory
	add esp, 20
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

_set_kernel_tbl_index:
	or ebx, FLAG_PRESENT|FLAG_READWRITE
	mov [eax+edx], ebx                    ; Use index from edx
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

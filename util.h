#include <stdint.h>

typedef struct gdt
{
} gdt_t;

typedef struct idt
{
} idt_t;

void setgdt(gdt_t* gdt, uint16_t gdtsize);
void setidt(idt_t* idt, uint16_t idtsize);

#include "idt.h" // Dołączamy nasz nagłówek

// Tutaj tworzymy faktyczne zmienne w pamięci
struct idt_entry idt[256];
struct idt_ptr idtp;

void _idt_set_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags) {
    // Wyciągamy dolne 16 bitów adresu (np. z 0x12345678 bierzemy 0x5678)
    idt[num].base_lo = (base & 0xFFFF);

    // Wyciągamy górne 16 bitów adresu (np. z 0x12345678 bierzemy 0x1234)
    idt[num].base_hi = (base >> 16) & 0xFFFF;

    idt[num].sel     = sel;
    idt[num].always0 = 0;
    idt[num].flags   = flags;
}

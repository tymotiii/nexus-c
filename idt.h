#ifndef IDT_H
#define IDT_H

// Szablony struktur (muszą tu być, żeby kernel.c wiedział ile ważą w pamięci)
// Struktura pojedynczego wpisu w tabeli IDT (musi mieć dokładnie 8 bajtów)
struct idt_entry {
    unsigned short base_lo;             // Niższe 16 bitów adresu handlera (ISR)
    unsigned short sel;                 // Selektor segmentu kodu w GDT (np. 0x08)
    unsigned char  always0;             // Ten bajt zawsze musi mieć wartość 0
    unsigned char  flags;               // Flagi bramki (np. 0x8E)
    unsigned short base_hi;             // Wyższe 16 bitów adresu handlera (ISR)
} __attribute__((packed));              // <--- TO JEST KLUCZOWE!

// Struktura opisująca wskaźnik IDT dla instrukcji LIDT (musi mieć dokładnie 6 bajtów)
struct idt_ptr {
    unsigned short limit;               // Rozmiar tabeli IDT minus 1
    unsigned int   base;                // Adres początkowy tabeli IDT w RAM
} __attribute__((packed));              // <--- TO JEST KLUCZOWE!

// !!! KLUCZOWE LINIJKI !!!
// Informujemy inne pliki, że te zmienne fizycznie żyją w pliku idt.c
extern struct idt_entry idt[256];
extern struct idt_ptr   idtp;

// Zapowiedź funkcji
void _idt_set_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags);
void _idt_init();

#endif

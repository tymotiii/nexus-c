#include <stdint.h>
#include "idt.h"
#include "syscalls/syscalls.h"

#define WIDTH 80
#define HEIGHT 25
static int cursor = 0;

__attribute__((section(".multiboot"), used))
unsigned int multiboot_header[] = {
    0x1BADB002,
    0x00000003,
    (unsigned int)(-(0x1BADB002 + 0x00000003))
};

void outb(unsigned short port, unsigned char data) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(data), "Nd"(port)
    );
}

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile (
        "inb %1, %0"
        : "=a"(result)
        : "Nd"(port)
    );
    return result;
}

// ------[TEXT MODE]-------
volatile char *video = (volatile char*)0xB8000;
volatile char last_char = 0;

char _getch() {
    while (last_char == 0) {
        __asm__ volatile("hlt");
    }
    char c = last_char;
    last_char = 0;
    return c;
}

void _terminal_set_cursor (int poss) {
    uint16_t pos = poss;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void clear_screen() {
    for (int i = 0; i < (WIDTH * HEIGHT); i++) {
        video[i] = 0x00;
        video[i+1] = 0x00;
    }
}


void _scroll() {
    for (int y = 1; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int dest_idx = ((y - 1) * WIDTH + x) * 2;
            int src_idx  = (y * WIDTH + x) * 2;
            video[dest_idx]     = video[src_idx];
            video[dest_idx + 1] = video[src_idx + 1];
        }
    }

    int last_row_start = (HEIGHT - 1) * WIDTH;
    for (int x = 0; x < WIDTH; x++) {
        int idx = (last_row_start + x) * 2;
        video[idx]     = ' ';
        video[idx + 1] = 0x07;
    }
    cursor = last_row_start;
}

void printk(const char *txt) {
    for (int i = 0; txt[i] != '\0'; i++) {
        if (cursor >= (WIDTH * HEIGHT)) {
            _scroll();
        }
        if (txt[i] == '\n') {
            cursor = ((cursor / WIDTH) + 1) * WIDTH;
            continue;
        }
        video[cursor * 2] = txt[i];
        video[cursor * 2 + 1] = 0x07;
        cursor++;
    }
    _terminal_set_cursor(cursor);
}

// -------[STRUKTURA REJESTRÓW]-------
// Dopasowana 1:1 do kolejności pushowania rejestrów w sekcji assemblerowej isr_common_stub
struct registers {
    unsigned int gs, fs, es, ds;                         // Pchnięte ręcznie w ASM stubie
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pchnięte przez PUSHAD
    unsigned int int_no, err_code;                       // Wepchnięte przez makra ISR
    unsigned int eip, cs, eflags, useresp, ss;           // Wepchnięte automatycznie przez procesor
};

// -------[ GDT STRUKTURY ]-------
struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char  base_middle;
    unsigned char  access;
    unsigned char  granularity;
    unsigned char  base_high;
} __attribute__((packed));

struct gdt_ptr {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));

struct gdt_entry gdt[6]; // Zmieniono rozmiar na 6 wpisów

struct tss_entry_struct {
    unsigned int link;
    unsigned int esp0;
    unsigned int ss0;
    unsigned int esp1, ss1, esp2, ss2, cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi, es, cs, ss, ds, fs, gs, ldt, trap, iomap_base;
} __attribute__((packed));

struct tss_entry_struct tss_entry;
struct gdt_ptr gdtp;

void _gdt_set_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access      = access;
}

void _gdt_init() {
    gdtp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdtp.base  = (unsigned int)&gdt;

    _gdt_set_gate(0, 0, 0, 0, 0);                // Pusty null deskryptor
    _gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kod Jądra (0x08)
    _gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Dane Jądra (0x10)
    _gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // Kod Użytkownika Ring 3 (0x1B)
    _gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // Dane Użytkownika Ring 3 (0x23)

    unsigned int tss_base = (unsigned int)&tss_entry;
    unsigned int tss_limit = sizeof(tss_entry);

    for(char *p = (char*)&tss_entry; p < (char*)&tss_entry + sizeof(tss_entry); p++) *p = 0;

    tss_entry.ss0  = 0x10;
    static unsigned char kernel_stack_buffer[4096];
    tss_entry.esp0 = (unsigned int)&kernel_stack_buffer[4095];

    // Rejestracja TSS w GDT pod indeksem 5 (0x28)
    _gdt_set_gate(5, tss_base, tss_limit, 0xE9, 0x00);

    __asm__ volatile(
        "lgdt %0\n"
        "ljmp $0x08, $.next\n"
        ".next:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : "m"(gdtp) : "eax"
    );

    __asm__ volatile("ltr %%ax" : : "a"(0x28 | 3));
}

#define PIC1          0x20
#define PIC2          0xA0
#define PIC1_COMMAND  PIC1
#define PIC1_DATA     (PIC1+1)
#define PIC2_COMMAND  PIC2
#define PIC2_DATA     (PIC2+1)

void _io_wait() {
    outb(0x80, 0);
}

void _pic_remap() {
    _io_wait();
    outb(PIC1_COMMAND, 0x11);
    _io_wait();
    outb(PIC2_COMMAND, 0x11);

    _io_wait();
    outb(PIC1_DATA, 0x20); // IRQ 0-7 -> INT 32-39
    _io_wait();
    outb(PIC2_DATA, 0x28); // IRQ 8-15 -> INT 40-47

    _io_wait();
    outb(PIC1_DATA, 0x04);
    _io_wait();
    outb(PIC2_DATA, 0x02);

    _io_wait();
    outb(PIC1_DATA, 0x01);
    _io_wait();
    outb(PIC2_DATA, 0x01);

    _io_wait();
    outb(PIC1_DATA, 0xFC); // Odmaskuj IRQ0 (Timer) i IRQ1 (Klawiatura)
    _io_wait();
    outb(PIC2_DATA, 0xFF);
}

void _pit_init(unsigned int hz) {
    unsigned int divisor = 1193182 / hz;
    outb(0x43, 0x36);
    outb(0x40, (unsigned char)(divisor & 0xFF));
    outb(0x40, (unsigned char)((divisor >> 8) & 0xFF));
}

unsigned int page_directory[1024] __attribute__((aligned(4096)));
unsigned int first_page_table[1024] __attribute__((aligned(4096)));

void _paging_init() {
    // Wszystkie wpisy w katalogu stron flagujemy jako 0x07 (User Mode dostępny)
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000007;
    }

    // Mapujemy pierwsze 4MB tożsamościowo (0x00000000 - 0x003FFFFF)
    for(unsigned int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 4096) | 7; // Obecny + Do zapisu + User Mode
    }

    page_directory[0] = ((unsigned int)first_page_table) | 7;

    __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory));
    unsigned int cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void isr32(); extern void isr33(); extern void isr128();

void _idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (unsigned int)&idt;

    for (int i = 0; i < 256; i++) {
        _idt_set_gate(i, 0, 0, 0);
    }

    _idt_set_gate(0,  (unsigned int)isr0,  0x08, 0x8E);
    _idt_set_gate(1,  (unsigned int)isr1,  0x08, 0x8E);
    _idt_set_gate(2,  (unsigned int)isr2,  0x08, 0x8E);
    _idt_set_gate(3,  (unsigned int)isr3,  0x08, 0x8E);
    _idt_set_gate(4,  (unsigned int)isr4,  0x08, 0x8E);
    _idt_set_gate(5,  (unsigned int)isr5,  0x08, 0x8E);
    _idt_set_gate(6,  (unsigned int)isr6,  0x08, 0x8E);
    _idt_set_gate(7,  (unsigned int)isr7,  0x08, 0x8E);
    _idt_set_gate(8,  (unsigned int)isr8,  0x08, 0x8E);
    _idt_set_gate(9,  (unsigned int)isr9,  0x08, 0x8E);
    _idt_set_gate(10, (unsigned int)isr10, 0x08, 0x8E);
    _idt_set_gate(11, (unsigned int)isr11, 0x08, 0x8E);
    _idt_set_gate(12, (unsigned int)isr12, 0x08, 0x8E);
    _idt_set_gate(13, (unsigned int)isr13, 0x08, 0x8E);
    _idt_set_gate(14, (unsigned int)isr14, 0x08, 0x8E);
    _idt_set_gate(15, (unsigned int)isr15, 0x08, 0x8E);
    _idt_set_gate(16, (unsigned int)isr16, 0x08, 0x8E);
    _idt_set_gate(17, (unsigned int)isr17, 0x08, 0x8E);
    _idt_set_gate(18, (unsigned int)isr18, 0x08, 0x8E);
    _idt_set_gate(19, (unsigned int)isr19, 0x08, 0x8E);
    _idt_set_gate(20, (unsigned int)isr20, 0x08, 0x8E);
    _idt_set_gate(21, (unsigned int)isr21, 0x08, 0x8E);
    _idt_set_gate(22, (unsigned int)isr22, 0x08, 0x8E);
    _idt_set_gate(23, (unsigned int)isr23, 0x08, 0x8E);
    _idt_set_gate(24, (unsigned int)isr24, 0x08, 0x8E);
    _idt_set_gate(25, (unsigned int)isr25, 0x08, 0x8E);
    _idt_set_gate(26, (unsigned int)isr26, 0x08, 0x8E);
    _idt_set_gate(27, (unsigned int)isr27, 0x08, 0x8E);
    _idt_set_gate(28, (unsigned int)isr28, 0x08, 0x8E);
    _idt_set_gate(29, (unsigned int)isr29, 0x08, 0x8E);
    _idt_set_gate(30, (unsigned int)isr30, 0x08, 0x8E);
    _idt_set_gate(31, (unsigned int)isr31, 0x08, 0x8E);

    // Przerwania sprzętowe muszą mieć DPL ustawione na Ring 0 (0x8E) lub Ring 3 (0xEE)
    _idt_set_gate(32, (unsigned int)isr32, 0x08, 0x8E);
    _idt_set_gate(33, (unsigned int)isr33, 0x08, 0x8E);
    _idt_set_gate(128, (unsigned int)isr128, 0x08, 0xEE);

    __asm__ volatile("lidt %0" : : "m"(idtp));
}

unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

struct task {
    unsigned int esp;
    unsigned int page_dir;
    unsigned int kernel_stack;
    unsigned int sleep_ticks;
    unsigned int state;
} __attribute__((packed));


// --- STRUKTURY MULTIBOOT ---
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count; // Liczba modułów (InitRD)
    uint32_t mods_addr;  // Adres tablicy modułów
} __attribute__((packed)) multiboot_info_t;

typedef struct {
    uint32_t mod_start;  // Adres początku pliku TAR w RAM
    uint32_t mod_end;    // Adres końca pliku TAR w RAM
    uint32_t cmdline;
    uint32_t pad;
} __attribute__((packed)) multiboot_module_t;

// --- STRUKTURA NAGŁÓWKA TAR (USTAR) ---
struct tar_header {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];     // Rozmiar w formacie ósemkowym (ASCII)
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
} __attribute__((packed));

// Zmienne globalne dla VFS
static uint32_t initrd_address = 0;
static uint32_t initrd_end_address = 0;


// Pomocnicza funkcja: proste porównanie stringów (jeśli nie masz string.h)
int kstrcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Konwersja tekstu ósemkowego z TAR na zwykłą liczbę całkowitą
static uint32_t _tar_parse_octal(const char *str, int size) {
    uint32_t n = 0;
    for (int i = 0; i < size && str[i] != '\0' && str[i] != ' '; i++) {
        n = n * 8 + (str[i] - '0');
    }
    return n;
}

// Inicjalizacja ("Montowanie") ramdisku w pamięci jądra
void vfs_mount_initrd(uint32_t start_addr, uint32_t end_addr) {
    initrd_address = start_addr;
    initrd_end_address = end_addr;
}

// Główna funkcja wyszukująca plik w InitRD
// Zwraca wskaźnik do surowych danych w pamięci RAM oraz ustawia out_size
char* vfs_read_file(const char* name, uint32_t* out_size) {
    if (initrd_address == 0) return 0;

    struct tar_header* header = (struct tar_header*)initrd_address;

    // Pętla leci po nagłówkach dopóki nie natrafi na pusty nagłówek (koniec pliku TAR)
    while (header->filename[0] != '\0') {
        // Sprawdź czy nie wyszliśmy poza bezpieczny obszar pamięci modułu
        if ((uint32_t)header >= initrd_end_address) break;

        uint32_t size = _tar_parse_octal(header->size, 12);

        // Jeśli nazwa pliku w TAR pokrywa się z szukaną
        if (kstrcmp(header->filename, name) == 0) {
            *out_size = size;
            // Dane pliku znajdują się dokładnie za 512-bajtowym nagłówkiem
            return ((char*)header) + 512;
        }

        // Przeskakujemy do następnego pliku w TAR:
        // Wielkość nagłówka (512) + wielkość pliku zaokrąglona w górę do pełnych bloków 512 bajtów
        uint32_t offset = 512 + ((size + 511) & ~511);
        header = (struct tar_header*)((uint32_t)header + offset);
    }

    return 0; // Plik nie istnieje
}

volatile int total_tasks = 0;
volatile struct task process_table[64];
volatile int current_task_id = 0;
volatile int scheduler_enabled = 0;
#define STATE_FREE    0  // Pusty slot w process_table, można tu stworzyć nowy proces
#define STATE_READY   1  // Proces żyje i chce działać (lub aktualnie działa)
#define STATE_SLEEP   2




void _create_user_task(int id, void (*entry_point)()) {
    static unsigned char task_kstack[64][4096];
    static unsigned char task_ustack[64][4096];

    unsigned int *stack = (unsigned int *)&task_kstack[id][4092];

    // Kontekst dla iret w Ring 3
    *(--stack) = 0x23;                                  // SS użytkownika (0x23)
    *(--stack) = (unsigned int)&task_ustack[id][4092];  // ESP użytkownika
    *(--stack) = 0x202;                                // EFLAGS: IF=1 (0x200) + IOPL=3 (0x3000) <--- KLUCZ
    *(--stack) = 0x1B;                                  // CS użytkownika (0x1B)
    *(--stack) = (unsigned int)entry_point;             // EIP zadania

    *(--stack) = 0;  // err_code
    *(--stack) = 32; // int_no

    // Pushad (eax, ecx, edx, ebx, esp, ebp, esi, edi)
    for(int i = 0; i < 8; i++) *(--stack) = 0;

    // Segmenty dla rejestrów segmentowych przywracanych przez ASM (gs, fs, es, ds)
    *(--stack) = 0x23; // gs
    *(--stack) = 0x23; // fs
    *(--stack) = 0x23; // es
    *(--stack) = 0x23; // ds

    process_table[id].esp = (unsigned int)stack;
    process_table[id].page_dir = (unsigned int)page_directory;
    process_table[id].kernel_stack = (unsigned int)&task_kstack[id][4092];
    process_table[id].state = STATE_READY;

    total_tasks++;
}

unsigned int interrupt_handler(struct registers *regs) {
    unsigned int current_esp = (unsigned int)regs;

    if (regs->int_no == 0) {
        printk("\n[IDT] Repairing divide by zero int...\n");
        regs->eip += 2;
        return current_esp;
    }

    // --- TIMER / SCHEDULER ---
    if (regs->int_no == 32) {
            // 1. Aktualizacja liczników uśpienia (poprawiony warunek)
            for (int i = 0; i < total_tasks; i++) {
                if (process_table[i].state == STATE_SLEEP) {
                    if (process_table[i].sleep_ticks > 0) {
                        process_table[i].sleep_ticks--;
                    }
                    if (process_table[i].sleep_ticks == 0) {
                        process_table[i].state = STATE_READY;
                    }
                }
            }

            if (scheduler_enabled && total_tasks > 0) {
                // Zapisujemy ESP bieżącego procesu TYLKO JEŚLI on nadal żyje!
                // Jeśli wywołał exit, to handle_syscall ustawiło mu stan 0 i nie chcemy nadpisywać bazy.
                if (process_table[current_task_id].state != STATE_FREE) {
                    process_table[current_task_id].esp = current_esp;
                }

                int next_task_id = current_task_id;
                int found_ready_task = 0;

                // Szukamy aktywnego procesu (STATE_READY)
                for (int i = 0; i < total_tasks; i++) {
                    next_task_id = (next_task_id + 1) % total_tasks;

                    if (process_table[next_task_id].state == STATE_READY) {
                        current_task_id = next_task_id;
                        found_ready_task = 1;
                        break;
                    }
                }

                // POPRAWIONY BEZPIECZNIK:
                // Jeśli żaden proces nie jest READY (wszystkie śpią lub zostały zabite),
                // nie możemy losowo skakać po indeksach! Musimy zostać na obecnym procesie (o ile żyje)
                if (!found_ready_task) {
                    if (process_table[current_task_id].state == STATE_FREE) {
                        // Krytyczny przypadek: obecny proces właśnie umarł, a reszta śpi.
                        // Musimy pętlić jądro do momentu aż timer obudzi jakiś proces (tzw. IDLE).
                        printk("\n[scheduler] Wszystko spi lub umarlo. Haltet!\n");
                        for(;;);
                    }
                    // Jeśli obecny proces żyje, po prostu na nim zostajemy i nic nie zmieniamy
                } else {
                    // Jeśli znaleźliśmy działający proces, ładujemy jego parametry
                    current_esp = process_table[current_task_id].esp;
                    tss_entry.esp0 = process_table[current_task_id].kernel_stack;
                }
            }

            outb(0x20, 0x20); // EOI do PIC
            return current_esp;
        }


    if (regs->int_no == 128) {
            // handle_syscall może zmienić current_esp (np. przy sleep)
            current_esp = handle_syscall(regs, current_esp);
            // Aktualizuj stos jądra dla nowego/bieżącego procesu
            tss_entry.esp0 = process_table[current_task_id].kernel_stack;
            return current_esp;
        }

    // --- KLAWIATURA ---
    if (regs->int_no == 33) {
        unsigned char scancode = inb(0x60);
        if (!(scancode & 0x80)) {
            char c = kbd_us[scancode];
            if (c > 0) {
                char str[2] = {c, '\0'};
                printk(str);
            }
        }
        outb(0x20, 0x20);
        return current_esp;
    }

    // Obsługa ewentualnych wyjątków (np. jeśli coś poszło nie tak)
    if (regs->int_no < 32) {
        printk("\n!!! KERNEL PANIC !!! Wyjatek nr: ");
        // Prymitywne wypisanie numeru błędu na ekran
        char num[3] = {'0' + (regs->int_no / 10), '0' + (regs->int_no % 10), '\0'};
        printk(num);
        printk("\n");
        for (;;);
    }

    if (regs->int_no >= 32) {
        outb(0x20, 0x20);
    }

    return current_esp;
}

void user_program_1() {
    syscall_print("A");
    syscall_sleep(10000);
    syscall_print("Exiting program A");
    syscall_exit();
}

void user_program_2() {
    while(1) {
        syscall_print("B");
        syscall_sleep(10000);
    }
}

void user_program_3() {
    while(1) {
        syscall_getpid();
    }
}

extern void pop_and_iret();

// ZMIANA: Zmieniamy sygnaturę _start, aby GRUB podawał parametry bezpośrednio na stosie
void _start(uint32_t boot_magic, uint32_t boot_mbi_addr) {

    // Krok 1: Tekst powitalny
    clear_screen(); // Opcjonalnie wyczyść ekran, by widzieć wszystko od góry
    printk("[nexus] kernel started. \n");

    _gdt_init();
    printk("[nexus] init GDT table. \n");

    _idt_init();
    printk("[nexus] init IDT table. \n");

    _pic_remap();
    _pit_init(10000);
    printk("[nexus] init PIC remap.\n");

    // --- PRZETWARZANIE INITRD (TERAZ PRZED STRONICOWANIEM) ---
    if (boot_magic == 0x2BADB002) {
        multiboot_info_t* mbi = (multiboot_info_t*)boot_mbi_addr;

        // Bit 3 w mbi->flags informuje o obecności modułów
        if (mbi->flags & (1 << 3)) {
            if (mbi->mods_count > 0) {
                multiboot_module_t* mod = (multiboot_module_t*)mbi->mods_addr;

                // Montujemy system plików przekazując fizyczne adresy z pamięci RAM
                vfs_mount_initrd(mod->mod_start, mod->mod_end);
                printk("[nexus] InitRD mounted successfully from RAM!\n");

                // --- TEST VFS ---
                uint32_t test_size = 0;
                char* test_data = vfs_read_file("hello.txt", &test_size);
                if (test_data != 0) {
                    printk("[VFS TEST] Zawartosc hello.txt: ");
                    // Bezpieczne wypisanie zawartości pliku
                    for(uint32_t j = 0; j < test_size; j++) {
                        char tmp[2] = {test_data[j], '\0'};
                        printk(tmp);
                    }
                    printk("\n");
                } else {
                    printk("[VFS TEST] Blad: Nie znaleziono pliku hello.txt w TAR!\n");
                }
            } else {
                printk("[nexus] Warning: No multiboot modules found!\n");
            }
        } else {
            printk("[nexus] Warning: Multiboot flags say no modules present!\n");
        }
    } else {
        printk("[nexus] Warning: Not booted by a Multiboot compliant bootloader!\n");
    }

    // Krok 3: Dopiero teraz, gdy skończyliśmy pracę z adresem z GRUB-a, bezpiecznie włączamy stronicowanie
    _paging_init();
    printk("[nexus] paging init\n");

    // Tworzenie procesów (Twoje istniejące zadania)
    _create_user_task(0, user_program_1);
    _create_user_task(1, user_program_2);
    _create_user_task(2, user_program_3);

    // Włączamy scheduler
    scheduler_enabled = 1;

    // Ustawiamy esp0 w TSS na stos pierwszego procesu
    tss_entry.esp0 = process_table[0].kernel_stack;

    // Skok do pierwszego procesu
    __asm__ volatile(
        "mov %0, %%esp\n"
        "jmp pop_and_iret\n"
        : : "r"(process_table[0].esp)
    );

    while (1) {}
}

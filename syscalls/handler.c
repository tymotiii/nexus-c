#include "syscalls.h"
#include <stdint.h>

struct tss_entry_struct {
    unsigned int link;
    unsigned int esp0;  // To pole jest modyfikowane przy syscall_exit
    unsigned int ss0;
    unsigned int esp1, ss1, esp2, ss2, cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi, es, cs, ss, ds, fs, gs, ldt, trap, iomap_base;
} __attribute__((packed));

extern void printk(const char *txt);
extern volatile int current_task_id;
extern volatile int total_tasks;
extern volatile struct task process_table[64];
extern struct tss_entry_struct tss_entry;
extern void outb(unsigned short port, unsigned char data);
extern unsigned char inb(unsigned short port);
extern uint8_t global_task_code_buffers[64][8192];
extern char kbd_getc();
#define STATE_FREE    0  // Pusty slot w process_table, można tu stworzyć nowy proces
#define STATE_READY   1  // Proces żyje i chce działać (lub aktualnie działa)
#define STATE_SLEEP   2

unsigned int handle_syscall(struct registers *regs, unsigned int current_esp) {

    // syscall 1: print
    if (regs->eax == 1) { // Twój numerek syscall_print
            char* user_ptr = (char*)regs->ebx;   // To jest np. 0x00000004

            // Sprawdzamy, czy proces pochodzi z InitRD (id > 0, załóżmy że proces 0 to jądro/kmain)
            if (current_task_id > 0) {
                // OBLICZAMY REALNY ADRES W PAMIĘCI:
                // Baza bufora tego procesu w jądru + offset przekazany przez użytkownika
                char* real_kernel_ptr = (char*)((unsigned int)&global_task_code_buffers[current_task_id][0] + (unsigned int)user_ptr);

                printk(real_kernel_ptr);
            } else {
                // Jeśli to wywołanie z jądra (które mapuje 1:1), drukuj normalnie
                printk(user_ptr);
            }
        }

    // syscall 2: getpid
    else if (regs->eax == 2) {
        regs->eax = current_task_id;
    }

    // syscall 3: sleep
    else if (regs->eax == 3) {
        unsigned int ticks_to_sleep = regs->ebx;

        // Zapisz stan bieżącego procesu i uśpij go
        process_table[current_task_id].state = 2;
        process_table[current_task_id].sleep_ticks = ticks_to_sleep;
        process_table[current_task_id].esp = current_esp;

        // Znajdź następny nieśpiący proces
        int next_task_id = current_task_id;
        int found_ready_task = 0;

        // 2. Szukamy nowego procesu
        for (int i = 0; i < total_tasks; i++) {
            next_task_id = (next_task_id + 1) % total_tasks;
            if (process_table[next_task_id].state == 1) { // 1 = STATE_READY
                current_task_id = next_task_id;
                found_ready_task = 1;
                break;
            }
        }

        if (!found_ready_task) {
            printk("\n[nexus] Wszystkie procesy zakrecone. Brak zadan. Haltet!\n");
            for(;;);
        }

        current_task_id = next_task_id; // Zmieniamy globalne ID

        // 3. Zwracamy ESP nowego procesu
        return process_table[current_task_id].esp;
    }
    else if (regs->eax == 4) { // <--- Po prostu zwykły IF (lub switch) na samym początku
            int code = regs->ebx;
            // 1. Zabijamy obecny proces
            process_table[current_task_id].exit_code = code;
            process_table[current_task_id].state = 0;

            int next_task_id = current_task_id;
            int found_ready_task = 0;

            // 2. Szukamy nowego procesu
            for (int i = 0; i < total_tasks; i++) {
                next_task_id = (next_task_id + 1) % total_tasks;
                if (process_table[next_task_id].state == 1) { // 1 = STATE_READY
                    current_task_id = next_task_id;
                    found_ready_task = 1;
                    break;
                }
            }

            if (!found_ready_task) {
                printk("\n[nexus] Wszystkie procesy zakrecone. Brak zadan. Haltet!\n");
                for(;;);
            }

            current_task_id = next_task_id; // Zmieniamy globalne ID

            // 3. Zwracamy ESP nowego procesu
            return process_table[current_task_id].esp;
        }
    else if (regs->eax == 5) {
        // Sprawdzamy czy w buforze jest jakiś znak
                char c = kbd_getc();

                if (c == 0) {
                    // Brak znaków! Proces musi poczekać.
                    // Cofamy EIP o 2 bajty (rozmiar instrukcji `int $0x80`),
                    // dzięki czemu po obudzeniu proces ponowi to samo żądanie syscalla!
                    regs->eip -= 2;

                    // Zapisujemy obecny stan rejestrów
                    process_table[current_task_id].esp = current_esp;

                    // Oddajemy procesor: Szukamy następnego gotowego zadania (skrócony scheduler)
                    int next_task_id = current_task_id;
                    for (int i = 0; i < total_tasks; i++) {
                        next_task_id = (next_task_id + 1) % total_tasks;
                        if (process_table[next_task_id].state == STATE_READY) {
                            current_task_id = next_task_id;
                            break;
                        }
                    }

                    // Ładujemy stos nowego procesu
                    current_esp = process_table[current_task_id].esp;
                }
                else {
                    // Znak jest dostępny! Zwracamy go do programu użytkownika przez rejestr EAX
                    regs->eax = (unsigned int)c;
                }
    }

    return current_esp;
}

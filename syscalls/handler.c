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
static unsigned int schedule_next_ready(void) {
    int next_task_id = current_task_id;
    for (int i = 0; i < total_tasks; i++) {
        next_task_id = (next_task_id + 1) % total_tasks;
        if (process_table[next_task_id].state == STATE_READY) {
            current_task_id = next_task_id;
            return process_table[current_task_id].esp;
        }
    }

    while (1) {
        __asm__ volatile("sti; hlt");
        for (int i = 0; i < total_tasks; i++) {
            if (process_table[i].state == STATE_READY) {
                current_task_id = i;
                return process_table[i].esp;
            }
        }
    }
}

unsigned int handle_syscall(struct registers *regs, unsigned int current_esp) {

    // syscall 1: print string (ebx = pointer inside the running process address space)
    if (regs->eax == 1) {
            printk((char *)regs->ebx);
        }

    // syscall 6: print one character (ebx = byte)
    else if (regs->eax == 6) {
            char buf[2] = {(char)(regs->ebx & 0xFF), '\0'};
            printk(buf);
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
        char c = kbd_getc();

        if (c == 0) {
            // Brak znaków: ponów ten sam int $0x80 po obudzeniu.
            regs->eip -= 2;
            process_table[current_task_id].esp = current_esp;
            process_table[current_task_id].state = STATE_BLOCKED;

            current_esp = schedule_next_ready();
        } else {
            process_table[current_task_id].state = STATE_READY;
            regs->eax = (unsigned int)(unsigned char)c;
        }
    }

    return current_esp;
}

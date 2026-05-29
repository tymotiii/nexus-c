CC = gcc
LD = ld
TARGET = kernel.elf
ISO_TARGET = os.iso
SRC = kernel.c
LINKER_SCRIPT = linker.ld

CFLAGS = -m32 -ffreestanding -fno-pie -fno-builtin -O0 -Wall -Wextra -g -lgcc
LDFLAGS = -m elf_i386 -T $(LINKER_SCRIPT) -nostdlib

OBJS = boot.o kernel.o video.o idt.o int.o syshandler.o sysusrapi.o

QEMU = qemu-system-x86_64
QEMU_OPTS = -machine pc,i8042=on -m 512M -vga std -cdrom $(ISO_TARGET) -boot order=d -no-reboot

# QEMU uses SeaBIOS by default; the ISO must include PC-BIOS GRUB (grub-pc-bin).
GRUB_BIOS ?= /usr/lib/grub/i386-pc

all: $(ISO_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

syshandler.o: syscalls/handler.c
	$(CC) $(CFLAGS) -c $< -o $@

sysusrapi.o: syscalls/usrapi.c
	$(CC) $(CFLAGS) -c $< -o $@

int.o: int.asm
	nasm -f elf32 int.asm -o int.o

boot.o: boot.S
	nasm -f elf32 boot.S -o boot.o

$(TARGET): $(OBJS) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) $(OBJS) -o $@
	grub-file --is-x86-multiboot $@ || (echo "ERROR: kernel.elf is not Multiboot1-valid"; exit 1)

programs:
	$(MAKE) -C programs

$(ISO_TARGET): $(TARGET) grub.cfg programs
	@test -f initrd/etc/testprog.bin || (echo "ERROR: missing initrd/etc/testprog.bin"; exit 1)
	@test -d "$(GRUB_BIOS)" || (echo "ERROR: $(GRUB_BIOS) not found."; \
		echo "Install BIOS GRUB modules: sudo apt install grub-pc-bin"; exit 1)
	mkdir -p iso/boot/grub
	tar -cf initrd.tar -C ./initrd .
	cp $(TARGET) iso/boot/$(TARGET)
	cp initrd.tar iso/boot/initrd.tar
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_TARGET) "$(GRUB_BIOS)" iso
	rm -rf iso
	rm -f initrd.tar

clean:
	rm -f *.o $(TARGET) $(ISO_TARGET) initrd.tar
	rm -rf iso

run: $(ISO_TARGET)
	$(QEMU) $(QEMU_OPTS)

debug: $(ISO_TARGET)
	$(QEMU) $(QEMU_OPTS) -d int,cpu_reset

CC = gcc
LD = ld
TARGET = kernel.elf
ISO_TARGET = os.iso
SRC = kernel.c
LINKER_SCRIPT = linker.ld

CFLAGS = -m32 -ffreestanding -fno-pie -fno-builtin -O0 -Wall -Wextra -g -lgcc
LDFLAGS = -m elf_i386 -T $(LINKER_SCRIPT) -nostdlib

# Pomocnicza lista wszystkich obiektów, żeby nie pisać tego ręcznie 3 razy
OBJS = kernel.o idt.o int.o syshandler.o sysusrapi.o

all: $(ISO_TARGET)

# Wspólna reguła kompilacji obiektów
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

syshandler.o: syscalls/handler.c
	$(CC) $(CFLAGS) -c $< -o $@

sysusrapi.o: syscalls/usrapi.c
	$(CC) $(CFLAGS) -c $< -o $@

int.o: int.asm
	nasm -f elf32 int.asm -o int.o

# Budowanie samego pliku jądra
$(TARGET): kernel.o idt.o int.o syshandler.o sysusrapi.o
	$(LD) $(LDFLAGS) $(OBJS) -o $(TARGET)

# Budowanie obrazu ISO (Główna, czysta reguła)
$(ISO_TARGET): $(TARGET) grub.cfg
	mkdir -p iso/boot/grub
	# Tworzymy poprawny plik TAR z zawartości folderu ./initrd
	tar -cf initrd.tar -C ./initrd .
	cp $(TARGET) iso/boot/$(TARGET)
	cp initrd.tar iso/boot/initrd.tar
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_TARGET) iso
	rm -rf iso
	rm -f initrd.tar

clean:
	rm -f *.o $(TARGET) $(ISO_TARGET) initrd.tar
	rm -rf iso

run: clean $(TARGET)
	mkdir -p iso/boot/grub
	# POPRAWKA: -C przechodzi do katalogu, a kropka pakuje tylko zawartość
	tar -cf initrd.tar -C ./initrd .
	cp $(TARGET) iso/boot/$(TARGET)
	cp initrd.tar iso/boot/initrd.tar
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_TARGET) iso
	rm -rf iso
	rm -f $(OBJS) $(TARGET) initrd.tar
	qemu-system-x86_64 -cdrom os.iso -boot d

debug: clean $(TARGET)
	mkdir -p iso/boot/grub
	# POPRAWKA: Usunięty błędny ukośnik '/', dodana flaga -C
	tar -cf initrd.tar -C ./initrd .
	cp $(TARGET) iso/boot/$(TARGET)
	cp initrd.tar iso/boot/initrd.tar
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_TARGET) iso
	rm -rf iso
	rm -f $(OBJS) initrd.tar
	qemu-system-x86_64 -cdrom os.iso -boot d -S -s

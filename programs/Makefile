# Kompilator i narzędzia
CXX = g++
LD = ld
OBJCOPY = objcopy

# Must match USER_CODE_BASE + process_id * 8192 in kernel.c (testprog uses id 1)
USER_LINK_BASE = 0x08002000

CXXFLAGS = -m32 -c -std=c++11 -ffreestanding -O2 -Wall -Wextra \
           -fno-exceptions -fno-rtti -fno-pie -fno-stack-protector

LDFLAGS = -m elf_i386 -Ttext $(USER_LINK_BASE) -e _start

OUT_DIR = ../initrd/etc

all:
	@mkdir -p $(OUT_DIR)
	@for file in *.cpp; do \
		name=$${file%.cpp}; \
		echo "[CXX] Kompilacja $$file..."; \
		$(CXX) $(CXXFLAGS) $$file -o $$name.o || exit 1; \
		echo "[LD] Linkowanie $$name.o @ $(USER_LINK_BASE)..."; \
		$(LD) $(LDFLAGS) $$name.o -o $$name.elf || exit 1; \
		echo "[OBJCOPY] Generowanie $$name.bin..."; \
		$(OBJCOPY) -O binary $$name.elf $$name.bin || exit 1; \
		echo "[MV] Przenoszenie $$name.bin do $(OUT_DIR)..."; \
		mv $$name.bin $(OUT_DIR)/; \
		rm -f $$name.o $$name.elf; \
	done
	@echo "[SUCCESS] Wszystkie pliki zostały przeniesione!"

clean:
	@echo "[CLEAN] Usuwanie plików .bin z $(OUT_DIR)..."
	@rm -f $(OUT_DIR)/*.bin

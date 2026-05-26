# Kompilator i narzędzia
CXX = g++
LD = ld
OBJCOPY = objcopy

# Flagi kompilacji dla C++ freestanding
CXXFLAGS = -m32 -c -std=c++11 -ffreestanding -O2 -Wall -Wextra \
           -fno-exceptions -fno-rtti -fno-pie -fno-stack-protector

# Wskazanie punktu wejścia jako _start
LDFLAGS = -m elf_i386 -Ttext 0x0 -e _start

# Ścieżka docelowa
OUT_DIR = ../initrd/etc

# Reguła główna - kompiluje wszystko po kolei
all:
	@mkdir -p $(OUT_DIR)
	@for file in *.cpp; do \
		name=$${file%.cpp}; \
		echo "[CXX] Kompilacja $$file..."; \
		$(CXX) $(CXXFLAGS) $$file -o $$name.o || exit 1; \
		echo "[LD] Linkowanie $$name.o..."; \
		$(LD) $(LDFLAGS) $$name.o -o $$name.elf || exit 1; \
		echo "[OBJCOPY] Generowanie $$name.bin..."; \
		$(OBJCOPY) -O binary $$name.elf $$name.bin || exit 1; \
		echo "[MV] Przenoszenie $$name.bin do $(OUT_DIR)..."; \
		mv $$name.bin $(OUT_DIR)/; \
		rm -f $$name.o $$name.elf; \
	done
	@echo "[SUCCESS] Wszystkie pliki zostały przeniesione!"

# Czyszczenie
clean:
	@echo "[CLEAN] Usuwanie plików .bin z $(OUT_DIR)..."
	@rm -f $(OUT_DIR)/*.bin

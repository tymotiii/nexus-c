#ifndef STDIO_H
#define STDIO_H

// Numery syscalli - DOPASUJ JE do swoich numerów w handle_syscall!
#define SYS_PRINT 1
#define SYS_READ  5
#define SYS_EXIT  4

// Makro ułatwiające wywoływanie syscalli w asemblerze inline
inline void syscall_print(const char* str) {
    __asm__ volatile("int $0x80" : : "a"(SYS_PRINT), "b"(str));
}

inline char syscall_read_char() {
    char c;
    __asm__ volatile("int $0x80" : "=a"(c) : "a"(SYS_READ));
    return c;
}

inline void syscall_exit(int code) {
    __asm__ volatile("int $0x80" : : "a"(SYS_EXIT), "b"(code));
}

namespace std {

    // Klasa symulująca std::cout
    class OutputStream {
    public:
        // Obsługa ciągów znaków
        OutputStream& operator<<(const char* str) {
            syscall_print(str);
            return *this;
        }

        // Obsługa pojedynczych znaków
        OutputStream& operator<<(char c) {
            char buf[2] = {c, '\0'};
            syscall_print(buf);
            return *this;
        }

        // Obsługa liczb całkowitych (automatyczna konwersja na tekst)
        OutputStream& operator<<(int value) {
            if (value == 0) return *this << "0";
            char buf[32];
            int i = 0;
            bool is_negative = false;
            if (value < 0) { is_negative = true; value = -value; }
            while (value > 0) { buf[i++] = (value % 10) + '0'; value /= 10; }
            if (is_negative) buf[i++] = '-';
            char reversed[32];
            int j = 0;
            while (i > 0) reversed[j++] = buf[--i];
            reversed[j] = '\0';
            syscall_print(reversed);
            return *this;
        }

        OutputStream& operator<<(unsigned int value) {
            if (value == 0) return *this << "0";
            char buf[32];
            int i = 0;
            while (value > 0) { buf[i++] = (value % 10) + '0'; value /= 10; }
            char reversed[32];
            int j = 0;
            while (i > 0) reversed[j++] = buf[--i];
            reversed[j] = '\0';
            syscall_print(reversed);
            return *this;
        }
    };

    // Klasa symulująca std::cin
    class InputStream {
    public:
        // Obsługa wczytywania pojedynczego znaku
        InputStream& operator>>(char& c) {
            c = syscall_read_char();
            return *this;
        }
    };

    // Globalne obiekty oznaczone jako INLINE, żeby linker nie wywalał błędu "multiple definition"
    inline OutputStream cout;
    inline InputStream cin;

    // Znak nowej linii jako stała inline
    inline const char endl = '\n';

    inline void exit(int status) {
        syscall_exit(status);
        // Bezpiecznik na wypadek, gdyby jądro nie zabiło procesu od razu
        while(1) { __asm__ volatile("hlt"); }
    }
}

// Funkcje pomocnicze w stylu języka C
extern "C" {
    inline void puts(const char* str) {
        syscall_print(str);
        syscall_print("\n");
    }

    inline char getch() {
        return syscall_read_char();
    }

    inline void exit(int status) {
        std::exit(status);
    }
}

#endif

#include "libs/stdion.h"

extern "C" void _start() {
    while (1) {
        char c = 0;

        // Wczytaj pojedynczy znak (blokuje dopóki nie klikniesz klawisza)
        std::cin >> c;

        // Wypisz pojedynczy znak bezpośrednio na ekran!
        // Operator << dla pojedynczego znaku 'char' w stdion.h nie używa wskaźników pamięci,
        // tylko bezpiecznie tworzy bufor wewnątrz jądra.
        std::cout << c;
    }

    std::exit(0);
}

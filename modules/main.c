#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "traitement.c"

    const int plage = 100;
int main() {
    int tirages = 1000000;
    int tabResultats[plage];
    //traitement_rand(tirages, plage, tabResultats);
    traitement_rand48(tirages, plage, tabResultats);

    printf("Résultats:\n");
    for (int i = 0; i < plage; i++) {
        printf("Valeur %d: %d occurrences\n", i, tabResultats[i]);
    }
    printf("Traitement terminé.\n");

    return 0;
}

int create_IPC() {
    return 0;
}
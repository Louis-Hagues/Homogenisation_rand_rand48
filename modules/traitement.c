#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// traitement avec 20 fork
int traitement_rand(int tirages, int plage, int* tabResultats) {
    int nbTiragesFork = tirages / 20;

    // initialisation du générateur aléatoire dans le parent
    srand(time(NULL));

    for (int i = 0; i < 20; i++) {

        if (fork() == 0) { // Processus fils
            // chaque fils doit avoir une seed différente
            srand(time(NULL) ^ (getpid() << 16));

            // traitement des tirages
            for (int j = 0; j < nbTiragesFork + (i < nbTiragesFork ? 1 : 0); j++) {
                int r = rand() % plage;
                tabResultats[r]++;
            }
            exit(0);
        }
    }
}


int traitement_rand48(int tirages, int plage, int* tabResultats) {
    int nbTiragesFork = tirages / 20;

    for (int i = 0; i < 20; i++) {
        if (fork() == 0) {   // Fils

            // Seed différente pour chaque processus
            srand48(time(NULL) ^ (getpid() << 16));

            for (int j = 0; j < nbTiragesFork + (i < nbTiragesFork ? 1 : 0); j++) {
                int r = lrand48() % plage;

                tabResultats[r]++;
            }
            exit(0);
        }
    }
}

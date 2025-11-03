// traitement avec 20 fork
int traitement_rand(int tirages, int plage, int* tabResultats) {
    int nbTiragesFork = tirages/20;

    //création des forks
    for (int i = 0; i < 20; i++) {
        if (fork() == 0) {
            //traitement des tirages
            for (int j = 0; j < nbTiragesFork + (i < nbTiragesFork ? 1 : 0); j++) {
                int r = rand(plage);
                tabResultats[r]++;
            }
            exit(0); //terminer le processus fils
        }
    }
}

int traitement_rand48(int tirages, int plage, int* tabResultats) {
    //traitement avec 20 fork
    int nbTiragesFork = tirages/20;

    //création des forks
    for (int i = 0; i < 20; i++) {
        if (fork() == 0) {
            //traitement des tirages
            for (int j = 0; j < nbTiragesFork + (i < nbTiragesFork ? 1 : 0); j++) {
                int r = rand48(plage);
                tabResultats[r]++;
            }
            exit(0); //terminer le processus fils
        }
    }
}
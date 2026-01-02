/* graphique.c
   simple fonctions d'affichage / statistique réutilisables
*/
#include <stdio.h>

void print_width_percent(long long total, int min, int max) {
    double largeur = (max > 0) ? ((double)(max - min) / (double)max) * 100.0 : 0.0;
    printf("Total tirages comptés: %lld\n", total);
    printf("Min occurrences: %d\n", min);
    printf("Max occurrences: %d\n", max);
    printf("Largeur ( (max-min)/max *100 ) = %.6f %%\n", largeur);
}

#ifndef TRAITEMENT_H
#define TRAITEMENT_H

#include <semaphore.h>
#include <stdint.h>

/**
 * @brief Configuration des paramètres de calcul
 */

/* Taille par défaut (1e9) */
#define TAB_SIZE_DEFAULT RAND_MAX

/* Nombre de processus fils à créer pour paralléliser le traitement */
#define NB_FILS 20

/* Itérations totales par machine (107,5 milliards) pour avoir plus de résultat */
// #define ITER_PER_MACHINE 107500000000LL

/* Itérations totales par machine (1 millions) pour test rapide */
#define ITER_PER_MACHINE 1000000LL

/**
 * @brief Exécute le calcul en parallèle en utilisant des processus fils.
 * * Cette fonction divise la charge de travail (ITER_PER_MACHINE) entre plusieurs
 * processus. Chaque fils incrémente les cases d'un tableau en mémoire partagée.
 *
 * @param shared_tab       Pointeur vers le tableau en mémoire partagée (IPC ou mmap).
 * @param tab_size         Nombre d'éléments dans le tableau.
 * @param iter_per_machine Nombre total d'itérations à répartir entre les fils.
 * @param rng_type         Sélecteur pour le type de générateur de nombres aléatoires.
 * * @return int             0 en cas de succès, une valeur négative en cas d'erreur.
 */
int run_parallel_shared(int *shared_tab, uint64_t tab_size, long long iter_per_machine, int rng_type);

/**
 * @brief Affiche les statistiques finales et la répartition des résultats.
 * * @param total            Nombre total d'itérations effectuées.
 * @param counts           Le tableau contenant les fréquences (résultats).
 * @param nb_classes       Taille du tableau 'counts'.
 */
void print_stats(long long total, const int *counts, int nb_classes);

#endif // TRAITEMENT_H

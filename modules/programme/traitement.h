#ifndef TRAITEMENT_H
#define TRAITEMENT_H

#include <semaphore.h>
#include <stdint.h>

/* Taille par défaut (1e9) */
#define TAB_SIZE_DEFAULT RAND_MAX

/* Nombre de fils à lancer */
#define NB_FILS 20

/* Itérations totales par machine (500 millions) */
#define ITER_PER_MACHINE 500000000LL

/* Prototypes */
int run_parallel_shared(int *shared_tab, uint64_t tab_size, long long iter_per_machine,
                        int rng_type, const char *sem_name);

void calc_stats_and_print(int *shared_tab, uint64_t tab_size, const char *label);

#endif // TRAITEMENT_H

#define _DEFAULT_SOURCE
#include "traitement.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>


static void child_work(int *shared_tab, uint64_t tab_size, long long iter_child, int rng_type) 
{
	unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
	if (rng_type == 0) {
		srand(seed);
	} else {
		srand48((long)seed);
	}

	for (long long i = 0; i < iter_child; ++i) {
		uint64_t r;
		if (rng_type == 0) {
			r = (uint64_t)     rand();
			r = r ^ ((uint64_t) rand() << 15);
			r = r % tab_size;
		} else {
			r = (uint64_t)lrand48() % tab_size;
		}
		__atomic_fetch_add(&shared_tab[r], 1, __ATOMIC_SEQ_CST);
	}
}

int run_parallel_shared(int *shared_tab, uint64_t tab_size, long long iter_per_machine,
						int rng_type, const char *sem_name) {
	if (!shared_tab) return -1;
	if (tab_size == 0) return -1;

	long long iter_per_child = iter_per_machine / NB_FILS;
	if (iter_per_child <= 0) {
		fprintf(stderr, "ITER per child <= 0\n");
		return -1;
	}

	pid_t pids[NB_FILS];
	for (int i = 0; i < NB_FILS; ++i) {
		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			//Attend les enfants
			for (int j = 0; j < i; ++j) wait(NULL);
			return -1;
		}
		if (pid == 0) {
			// Enfant
			child_work(shared_tab, tab_size, iter_per_child, rng_type);
			_exit(0);
		} else {
			pids[i] = pid;
		}
	}

	for (int i = 0; i < NB_FILS; ++i) {
		wait(NULL);
	}
	return 0;
}

void calc_stats_and_print(int *shared_tab, uint64_t tab_size, const char *label) {
	if (!shared_tab) return;

	long long total = 0;
	int min = shared_tab[0], max = shared_tab[0];
	for (uint64_t i = 0; i < tab_size; ++i) {
		int v = shared_tab[i];
		if (v < min) min = v;
		if (v > max) max = v;
		total += v;
	}
	double avg = (double)total / (double)tab_size;
	double largeur = (max > 0) ? ((double)(max - min) / (double)max) * 100.0 : 0.0;

	printf("\n=== Stats (%s) ===\n", label ? label : "résultat");
	printf("Tab size: %lu\n", (unsigned long)tab_size);
	printf("Total tirages comptés: %lld\n", total);
	printf("Min: %d\n", min);
	printf("Max: %d\n", max);
	printf("Average: %.6f\n", avg);
	printf("Largeur ( (max-min)/max *100 ) = %.6f %%\n", largeur);

	// preview first 10 cells
	printf("\nAperçu shared[0..9]:\n");
	uint64_t show = tab_size < 10 ? tab_size : 10;
	for (uint64_t i = 0; i < show; ++i) printf(" [%3lu] = %d\n", (unsigned long)i, shared_tab[i]);
}

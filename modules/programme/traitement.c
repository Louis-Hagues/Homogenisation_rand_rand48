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
#include <math.h>
#include <limits.h>


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
						int rng_type) {
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

void print_stats(long long total, const int *counts, int nb_classes)
{
	int min = INT_MAX;
	int max = 0;
	int zero_count = 0;

	double mean = (double)total / nb_classes;
	double variance = 0.0;
	double chi2 = 0.0;

	/* Parcours des classes */
	for (int i = 0; i < nb_classes; i++) {
		int c = counts[i];

		if (c == 0)
			zero_count++;

		if (c < min)
			min = c;
		if (c > max)
			max = c;

		double diff = c - mean;
		variance += diff * diff;

		if (mean > 0.0)
			chi2 += (diff * diff) / mean;
	}

	variance /= nb_classes;
	double stddev = sqrt(variance);
	double cv = (mean > 0.0) ? stddev / mean : 0.0;

	double largeur = (max > 0) ? ((double)(max - min) / (double)max) * 100.0 : 0.0;

	/* Affichage */
	printf("Total tirages comptés : %lld\n", total);
	printf("Nombre de classes     : %d\n", nb_classes);
	printf("Classes vides         : %d\n", zero_count);
	printf("Min occurrences       : %d\n", min);
	printf("Max occurrences       : %d\n", max);

	printf("Largeur naive         : %.6f %%\n", largeur);

	printf("Moyenne par classe    : %.2f\n", mean);
	printf("Écart-type            : %.4f\n", stddev);
	printf("Coefficient variation : %.4f (%.2f %%)\n", cv, cv * 100.0);

	printf("Chi²                  : %.2f\n", chi2);
	printf("Degrés de liberté     : %d\n", nb_classes - 1);
}
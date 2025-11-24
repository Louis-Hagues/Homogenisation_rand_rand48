#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <limits.h>
#include <wait.h>

#include "traitement.h"

/* Variables globales */
int RANDOM_MAX  = 10000;
int NB_TIRAGES  = 1000000000;

/* Prototypes */
int *create_IPC(int *out_shmid);
void destroy_IPC(int *ptr, int shmid);

/* ---------------------------------------------------------- */
/*  Création du segment de mémoire partagée                    */
/* ---------------------------------------------------------- */
int *create_IPC(int *out_shmid)
{
	key_t key = ftok("/tmp", 65);
	if (key == -1) {
		perror("ftok");
		exit(EXIT_FAILURE);
	}

	int shmid = shmget(key, (RANDOM_MAX + 1) * sizeof(int),
					   IPC_CREAT | 0666);
	if (shmid == -1) {
		perror("shmget");
		exit(EXIT_FAILURE);
	}

	int *shared = (int*) shmat(shmid, NULL, 0);
	if (shared == (void*) -1) {
		perror("shmat");
		exit(EXIT_FAILURE);
	}

	*out_shmid = shmid;
	return shared;
}

/* ---------------------------------------------------------- */
/*  Libération du segment de mémoire partagée                 */
/* ---------------------------------------------------------- */
void destroy_IPC(int *ptr, int shmid)
{
	if (ptr != NULL) {
		if (shmdt(ptr) == -1)
			perror("shmdt");
	}

	if (shmid != -1) {
		if (shmctl(shmid, IPC_RMID, NULL) == -1)
			perror("shmctl");
	}
}


int main(void)
{
	int shmid;
	int *shared = create_IPC(&shmid);

	/* Initialisation */
	for (int i = 0; i <= RANDOM_MAX; ++i)
		shared[i] = 0;

	/* ------------------------------------------------------ */
	/*                 Appel du traitement                    */
	/* ------------------------------------------------------ */
	traitement_rand(
		NB_TIRAGES,      // total de tirages
		RANDOM_MAX + 1,  // plage possible
		shared           // mémoire partagée
	);


	for (int i = 0; i < 20; i++)
		wait(NULL);


	long total_counts = 0;
	int non_zero = 0;

	for (int i = 0; i <= RANDOM_MAX; ++i) {
		total_counts += shared[i];
		if (shared[i] != 0)
			non_zero++;
	}

	printf("Tirages demandés  = %d\n", NB_TIRAGES);
	printf("Compteurs totaux  = %ld\n", total_counts);
	printf("Valeurs non nulles= %d sur %d\n",
		   non_zero, RANDOM_MAX + 1);

	printf("\nAperçu shared[0..9]:\n");
	for (int i = 0; i <= RANDOM_MAX; ++i)
		printf("  [%3d] = %d\n", i, shared[i]);

	/* Nettoyage */
	destroy_IPC(shared, shmid);

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "traitement.c"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <wait.h>

/* Variables globales */
int RANDOM_MAX  = 1000;
int NB_TIRAGES  = 1000000;
int NB_CHILDREN = 2;

/* Prototypes */
int *create_IPC(int *out_shmid);
void destroy_IPC(int *ptr, int shmid);

int *create_IPC(int *out_shmid)
{
    key_t key = ftok("/tmp", 65);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(key, (RANDOM_MAX + 1) * sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    int *shared = (int*) shmat(shmid, NULL, 0);
    if (shared == (void*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    *out_shmid = shmid;   // on renvoie le shmid au main

    return shared;
}


void destroy_IPC(int *ptr, int shmid)
{
    if (ptr != NULL) {
        if (shmdt(ptr) == -1)
            perror("shmdt null");
    }

    if (shmid != -1) {
        if (shmctl(shmid, IPC_RMID, NULL) == -1)
            perror("shmctl IPC_RMID");
    }
}

int main(void)
{
    int shmid;  
    int *shared = create_IPC(&shmid);

    /* Initialise la mémoire */
    for (int i = 0; i <= RANDOM_MAX; ++i)
        shared[i] = 0;

    /* 2) Forks */
    pid_t pids[NB_CHILDREN];
    int tirages_par_child = NB_TIRAGES / NB_CHILDREN;

    for (int c = 0; c < NB_CHILDREN; ++c) {

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            for (int k = 0; k < c; ++k)
                waitpid(pids[k], NULL, 0);
            destroy_IPC(shared, shmid);
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            /* -------- CODE FILS -------- */

            int chunk_size = (RANDOM_MAX + 1) / NB_CHILDREN;
            int start = c * chunk_size;
            int end   = (c == NB_CHILDREN - 1) ? RANDOM_MAX : (start + chunk_size - 1);

            srand((unsigned)(time(NULL) ^ (getpid()<<16)));

            for (int t = 0; t < tirages_par_child; ++t) {
                int v = rand() % (RANDOM_MAX + 1);
                if (v >= start && v <= end) {
                    shared[v] += 1;
                }
            }

            /* Le fils se détache puis termine */
            shmdt(shared);
            _exit(0);
        }

        /* Parent */
        pids[c] = pid;
    }

    /* 3) Attendre tous les fils */
    for (int c = 0; c < NB_CHILDREN; ++c) {
        waitpid(pids[c], NULL, 0);
    }

    /* 4) Analyse */
    long total_counts = 0;
    int non_zero = 0;

    for (int i = 0; i <= RANDOM_MAX; ++i) {
        total_counts += shared[i];
        if (shared[i] != 0)
            non_zero++;
    }

    printf("Tirages demandés  = %d\n", NB_TIRAGES);
    printf("Compteurs totaux  = %ld\n", total_counts);
    printf("Valeurs non nulles= %d sur %d\n", non_zero, RANDOM_MAX+1);

    printf("\nAperçu shared[0..9]:\n");
    for (int i = 0; i < 10 && i <= RANDOM_MAX; ++i)
        printf("  [%3d] = %d\n", i, shared[i]);

    /* 5) Nettoyage */
    destroy_IPC(shared, shmid);
int *create_IPC(int *out_shmid);
void destroy_IPC(int *ptr, int shmid);

int *create_IPC(int *out_shmid)
{
    key_t key = ftok("/tmp", 65);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(key, (RANDOM_MAX + 1) * sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    int *shared = (int*) shmat(shmid, NULL, 0);
    if (shared == (void*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    *out_shmid = shmid;   // on renvoie le shmid au main

    return shared;
}


void destroy_IPC(int *ptr, int shmid)
{
    if (ptr != NULL) {
        if (shmdt(ptr) == -1)
            perror("shmdt null");
    }

    if (shmid != -1) {
        if (shmctl(shmid, IPC_RMID, NULL) == -1)
            perror("shmctl IPC_RMID");
    }
}

int main(void)
{
    int shmid;  
    int *shared = create_IPC(&shmid);

    /* Initialise la mémoire */
    for (int i = 0; i <= RANDOM_MAX; ++i)
        shared[i] = 0;

    /* 2) Forks */
    pid_t pids[NB_CHILDREN];
    int tirages_par_child = NB_TIRAGES / NB_CHILDREN;

    for (int c = 0; c < NB_CHILDREN; ++c) {

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            for (int k = 0; k < c; ++k)
                waitpid(pids[k], NULL, 0);
            destroy_IPC(shared, shmid);
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            /* -------- CODE FILS -------- */

            int chunk_size = (RANDOM_MAX + 1) / NB_CHILDREN;
            int start = c * chunk_size;
            int end   = (c == NB_CHILDREN - 1) ? RANDOM_MAX : (start + chunk_size - 1);

            srand((unsigned)(time(NULL) ^ (getpid()<<16)));

            for (int t = 0; t < tirages_par_child; ++t) {
                int v = rand() % (RANDOM_MAX + 1);
                if (v >= start && v <= end) {
                    shared[v] += 1;
                }
            }

            /* Le fils se détache puis termine */
            shmdt(shared);
            _exit(0);
        }

        /* Parent */
        pids[c] = pid;
    }

    /* 3) Attendre tous les fils */
    for (int c = 0; c < NB_CHILDREN; ++c) {
        waitpid(pids[c], NULL, 0);
    }

    /* 4) Analyse */
    long total_counts = 0;
    int non_zero = 0;

    for (int i = 0; i <= RANDOM_MAX; ++i) {
        total_counts += shared[i];
        if (shared[i] != 0)
            non_zero++;
    }

    printf("Tirages demandés  = %d\n", NB_TIRAGES);
    printf("Compteurs totaux  = %ld\n", total_counts);
    printf("Valeurs non nulles= %d sur %d\n", non_zero, RANDOM_MAX+1);

    printf("\nAperçu shared[0..9]:\n");
    for (int i = 0; i < 10 && i <= RANDOM_MAX; ++i)
        printf("  [%3d] = %d\n", i, shared[i]);

    /* 5) Nettoyage */
    destroy_IPC(shared, shmid);

    return 0;
}

int create_IPC() {
    return 0;
}
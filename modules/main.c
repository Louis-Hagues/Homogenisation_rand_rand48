#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

int RANDOM_MAX  = 1000;
int NB_TIRAGES = 1000;
int NB_CHILDREN = 2;

int *create_IPC()
{
	// Génère une clé unique à partir d’un fichier existant
	key_t key = ftok("/tmp", 65); // "/tmp" = chemin, 65 = identifiant arbitraire
	if (key == -1) {
		perror("Erreur ftok");
		exit(EXIT_FAILURE);
	}

	// Crée un segment de mémoire partagée
	int id_ipc = shmget(key, (RANDOM_MAX + 1) * sizeof(int), IPC_CREAT | 0666);
	if (id_ipc == -1) {
		perror("Erreur shmget");
		exit(EXIT_FAILURE);
	}

	// Attache le segment à l’espace mémoire du processus
	int *shared = (int*) shmat(id_ipc, NULL, 0);
	if (shared == (void*) -1) {
		perror("Erreur shmat");
		exit(EXIT_FAILURE);
	}

	return (int *) shared; // On retourne le pointeur vers la mémoire partagée
}

int main(void)
{
    /* 1) Créer / attacher la mémoire partagée (tableau int[RANDOM_MAX+1]) */
    int *shared = create_IPC();
    if (shared == NULL) {
        fprintf(stderr, "Échec création mémoire partagée\n");
        return EXIT_FAILURE;
    }

    /* (create_IPC a pu initialiser la zone à zéro; si pas le cas, on la met à zéro ici) */
    for (int i = 0; i <= RANDOM_MAX; ++i) shared[i] = 0;

    /* 2) Lancer les forks */
    pid_t pids[NB_CHILDREN];
    int tirages_par_child = NB_TIRAGES / NB_CHILDREN;
    for (int c = 0; c < NB_CHILDREN; ++c) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            /* Si fork échoue, attendre les enfants déjà créés puis quitter proprement */
            for (int k = 0; k < c; ++k) waitpid(pids[k], NULL, 0);
            destroy_IPC(shared);
            return EXIT_FAILURE;
        }
        if (pid == 0) {
            /* ----------- Code du fils ----------- */
            /* Déterminer la plage de valeurs qui lui est assignée (disjointe) */
            int chunk_size = (RANDOM_MAX + 1) / NB_CHILDREN;
            int start = c * chunk_size;
            int end = (c == NB_CHILDREN - 1) ? RANDOM_MAX : (start + chunk_size - 1);

            /* Seed unique pour chaque fils */
            srand((unsigned)(time(NULL) ^ (getpid()<<16)));

            /* Effectuer les tirages et n'incrémenter que si la valeur tombe dans sa plage */
            for (int t = 0; t < tirages_par_child; ++t) {
                int v = rand() % (RANDOM_MAX + 1);
                if (v >= start && v <= end) {
                    /* écriture sûre : chaque fils écrit uniquement dans sa plage */
                    shared[v] += 1;
                }
            }

            /* Détacher la mémoire partagée dans le fils et quitter */
            if (shmdt((void *)shared) == -1) {
                perror("shmdt (fils)");
                _exit(EXIT_FAILURE);
            }
            _exit(EXIT_SUCCESS);
        } else {
            /* Parent garde la PID pour waitpid plus tard */
            pids[c] = pid;
        }
    }

    /* 3) Parent attend la fin de tous les fils */
    for (int c = 0; c < NB_CHILDREN; ++c) {
        int status;
        if (waitpid(pids[c], &status, 0) == -1) {
            perror("waitpid");
        } else {
            if (WIFEXITED(status)) {
                /* enfant terminé normalement */
            } else {
                fprintf(stderr, "Enfant %d terminé anormalement\n", pids[c]);
            }
        }
    }

    /* 4) Analyse simple : somme des comptes et affichage d'un aperçu */
    long total_counts = 0;
    int non_zero = 0;
    for (int i = 0; i <= RANDOM_MAX; ++i) {
        total_counts += shared[i];
        if (shared[i] != 0) ++non_zero;
    }

    printf("Tirages demandés (approx) = %d\n", NB_TIRAGES);
    printf("Somme des compteurs = %ld\n", total_counts);
    printf("Nombre de valeurs non-nulles = %d (sur %d possibles)\n", non_zero, RANDOM_MAX + 1);

    /* Afficher un petit extrait des compteurs (0..9) pour vérification visuelle */
    printf("Extrait shared[0..9]:\n");
    for (int i = 0; i < 10 && i <= RANDOM_MAX; ++i) {
        printf("  [%3d] = %d\n", i, shared[i]);
    }

    /* 5) Nettoyage : détacher et supprimer la mémoire partagée */
    destroy_IPC(shared);

    return EXIT_SUCCESS;
}



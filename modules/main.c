#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>

#include "programme/traitement.h"

int create_server_socket(int port, int backlog);
int accept_client(int server_sock);
int create_client_and_connect(const char *server_ip, int port);
ssize_t send_all(int fd, const void *buf, size_t len);
ssize_t recv_all(int fd, void *buf, size_t len);
int get_local_ip(char *out, size_t outlen);

/* constants */
#define DEFAULT_PORT 5000
#define SHM_PERMISSIONS (0600)
#define SEMNAME_SERVER "/sem_mc_project_v1"

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage:\n  %s server [port]\n  %s client <server_ip> [port]\n", argv[0], argv[0]);
		return 1;
	}

	int is_server = (strcmp(argv[1], "server") == 0);
	int is_client = (strcmp(argv[1], "client") == 0);

	if (!is_server && !is_client) {
		fprintf(stderr, "Choisir 'server' ou 'client'\n");
		return 1;
	}

	int port = DEFAULT_PORT;
	if (is_server) {
		if (argc >= 3) port = atoi(argv[2]);
	} else {
		if (argc < 3) { fprintf(stderr, "Client: ip serveur requis\n"); return 1; }
		if (argc >= 4) port = atoi(argv[3]);
	}


	uint64_t tab_size = (uint64_t)TAB_SIZE_DEFAULT;
	printf("TAB_SIZE = %lu (elements) -> ~%.2f GB for int32 array\n",
		   (unsigned long)tab_size,
		   (double)tab_size * sizeof(int) / (1024.0*1024.0*1024.0));
	if (tab_size > 200000000UL) {
		printf("ATTENTION: TAB_SIZE très grand. Assure-toi d'avoir suffisamment de RAM.\n");
	}

	if (is_server) {
		char local_ip[64] = {0};
		if (get_local_ip(local_ip, sizeof(local_ip)) == 0) {
			printf("IP du serveur : %s\n", local_ip);
		} else {
			printf("Impossible d'obtenir IP locale.\n");
		}

		int server_sock = create_server_socket(port, 1);
		if (server_sock < 0) return 1;
		printf("Serveur: en attente d'un client sur le port %d...\n", port);
		int client_fd = accept_client(server_sock);
		if (client_fd < 0) { close(server_sock); return 1; }
		printf("Client connecté.\n");


		int rng_choice = 0;
		printf("Choisir RNG (0 = rand, 1 = drand48) : ");
		if (scanf("%d", &rng_choice) != 1) rng_choice = 0;

		int32_t rc_net = htonl(rng_choice);
		if (send_all(client_fd, &rc_net, sizeof(rc_net)) < 0) {
			perror("send rng_choice");
			close(client_fd); close(server_sock);
			return 1;
		}
		printf("RNG envoyé au client: %d\n", rng_choice);


		int shmid = shmget(IPC_PRIVATE, (size_t)tab_size * sizeof(int), IPC_CREAT | SHM_PERMISSIONS);
		if (shmid < 0) { perror("shmget"); close(client_fd); close(server_sock); return 1; }
		int *shared = (int*) shmat(shmid, NULL, 0);
		if (shared == (void*)-1) { perror("shmat"); shmctl(shmid, IPC_RMID, NULL); close(client_fd); close(server_sock); return 1; }
		memset(shared, 0, (size_t)tab_size * sizeof(int));

		sem_t *sem = sem_open(SEMNAME_SERVER, O_CREAT, 0644, 1);
		if (sem == SEM_FAILED) 
		{ 
			perror("sem_open"); 
			shmdt(shared); 
			shmctl(shmid, IPC_RMID, NULL); 
			close(client_fd); 
			close(server_sock); 
			return 1; 
		}

		printf("Serveur: lancement du traitement local (500M iterations)...\n");
		if (run_parallel_shared(shared, tab_size, ITER_PER_MACHINE, rng_choice, SEMNAME_SERVER) != 0) {
			fprintf(stderr, "Erreur traitement serveur\n");
		} else {
			printf("Serveur: traitement local terminé.\n");
		}

		printf("Serveur: attente des données du client...\n");

		const uint64_t block_elems = 1000000UL;
		uint64_t elems_remaining = tab_size;
		uint64_t idx = 0;
		uint32_t *buf = malloc(block_elems * sizeof(uint32_t));
		if (!buf) { perror("malloc recv buf");}

		while (elems_remaining > 0) {
			uint64_t this_block = (elems_remaining > block_elems) ? block_elems : elems_remaining;
			size_t bytes = this_block * sizeof(uint32_t);
			if (recv_all(client_fd, buf, bytes) < 0) {
				perror("recv_all");
				free(buf);
				break;
			}

			for (uint64_t i = 0; i < this_block; ++i) {
				uint32_t v = ntohl(buf[i]);
				__atomic_fetch_add(&shared[idx + i], (int)v, __ATOMIC_SEQ_CST);
			}
			idx += this_block;
			elems_remaining -= this_block;
		}
		free(buf);

		calc_stats_and_print(shared, tab_size, "agrégé (server+client)");


		shmdt(shared);
		shmctl(shmid, IPC_RMID, NULL);
		sem_close(sem);
		sem_unlink(SEMNAME_SERVER);
		close(client_fd);
		close(server_sock);
		return 0;
	}
	else { /* client */
		const char *server_ip = argv[2];
		int port_client = port;
		int sock = create_client_and_connect(server_ip, port_client);
		if (sock < 0) return 1;
		printf("Connecté au serveur %s:%d\n", server_ip, port_client);


		int32_t rc_net;
		if (recv_all(sock, &rc_net, sizeof(rc_net)) < 0) {
			perror("recv rng_choice");
			close(sock);
			return 1;
		}
		int rng_choice = (int)ntohl(rc_net);
		printf("RNG choisi par serveur: %d (%s)\n", rng_choice, rng_choice ? "drand48" : "rand");

		int shmid = shmget(IPC_PRIVATE, (size_t)tab_size * sizeof(int), IPC_CREAT | SHM_PERMISSIONS);
		if (shmid < 0) { perror("shmget client"); close(sock); return 1; }
		int *shared = (int*) shmat(shmid, NULL, 0);
		if (shared == (void*)-1) 
		{ 
			perror("shmat client"); 
			shmctl(shmid, IPC_RMID, NULL); 
			close(sock); 
			return 1; 
		}
		memset(shared, 0, (size_t)tab_size * sizeof(int));

		sem_t *sem = sem_open(SEMNAME_SERVER, O_CREAT, 0644, 1);
		if (sem == SEM_FAILED) 
		{ 
			perror("sem_open client"); 
			shmdt(shared); 
			shmctl(shmid, IPC_RMID, NULL); 
			close(sock); 
			return 1; 
		}

		printf("Client: lancement du traitement local (500M iterations)...\n");
		if (run_parallel_shared(shared, tab_size, ITER_PER_MACHINE, rng_choice, SEMNAME_SERVER) != 0) {
			fprintf(stderr, "Erreur traitement client\n");
		} else {
			printf("Client: traitement local terminé.\n");
		}

		const uint64_t block_elems = 1000000UL;
		uint64_t elems_remaining = tab_size;
		uint64_t idx = 0;
		uint32_t *buf = malloc(block_elems * sizeof(uint32_t));
		if (!buf) { perror("malloc send buf"); close(sock); shmdt(shared); shmctl(shmid, IPC_RMID, NULL); return 1; }

		while (elems_remaining > 0) {
			uint64_t this_block = (elems_remaining > block_elems) ? block_elems : elems_remaining;
			for (uint64_t i = 0; i < this_block; ++i) {
				buf[i] = htonl((uint32_t)shared[idx + i]);
			}
			size_t bytes = this_block * sizeof(uint32_t);
			if (send_all(sock, buf, bytes) < 0) {
				perror("send_all");
				free(buf);
				break;
			}
			idx += this_block;
			elems_remaining -= this_block;
		}
		free(buf);

		calc_stats_and_print(shared, tab_size, "client local");

		shmdt(shared);
		shmctl(shmid, IPC_RMID, NULL);
		sem_close(sem);
		close(sock);
		return 0;
	}
}

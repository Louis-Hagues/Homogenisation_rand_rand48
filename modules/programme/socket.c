#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>

#include "traitement.h"

/**
 * @brief Prépare un socket serveur prêt à accepter des connexions.
 * * @param port    Le numéro de port sur lequel écouter.
 * @param backlog Nombre maximal de connexions en attente dans la file.
 * @return int    Le descripteur du socket serveur ou -1 en cas d'erreur.
 */
int create_server_socket(int port, int backlog) {
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) { perror("socket"); return -1; }

	/* SO_REUSEADDR permet de relancer le serveur immédiatement sans attendre 
       que l'OS libère le port (évite l'erreur "Address already in use") */
	int opt = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; // Écoute sur toutes les interfaces réseau
	addr.sin_port = htons(port); // Convertit le port en format réseau (Big Endian)

	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(s);
		return -1;
	}
	if (listen(s, backlog) < 0) {
		perror("listen");
		close(s);
		return -1;
	}
	return s;
}

/**
 * @brief Attend et accepte la connexion d'un client.
 * * @param server_sock Le socket serveur précédemment créé.
 * @return int         Le socket de communication dédié au client connecté.
 */
int accept_client(int server_sock) {
	struct sockaddr_in cli;
	socklen_t len = sizeof(cli);
	int c = accept(server_sock, (struct sockaddr*)&cli, &len);
	if (c < 0) { perror("accept"); return -1; }
	return c;
}

/**
 * @brief Se connecte à un serveur distant via son adresse IP.
 */
int create_client_and_connect(const char *server_ip, int port) 
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) { perror("socket"); return -1; }

	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	/* Convertit l'adresse IP (chaîne de caractères) en format binaire réseau */
	if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
		fprintf(stderr, "inet_pton invalide: %s\n", server_ip);
		close(s);
		return -1;
	}
	if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(s);
		return -1;
	}
	return s;
}

/**
 * @brief Envoie l'intégralité d'un buffer, même si l'envoi est fragmenté.
 * * En réseau, send() peut envoyer moins d'octets que demandé.
 * On boucle jusqu'à ce que tout soit envoyé.
 */
ssize_t send_all(int fd, const void *buf, size_t len) {
	size_t total = 0;
	const char *p = buf;
	while (total < len) {
		ssize_t s = send(fd, p + total, len - total, 0);
		if (s < 0) {
			/* EINTR signifie qu'un signal a interrompu l'appel, on recommence */
			if (errno == EINTR) continue;
			return -1;
		}
		total += (size_t)s;
	}
	return (ssize_t)total;
}

/**
 * @brief Reçoit exactement le nombre d'octets demandé.
 * * Recv() peut renvoyer des données partielles. On force la réception
 * complète pour garantir l'intégrité des structures de données transmises.
 */
ssize_t recv_all(int fd, void *buf, size_t len) {
	size_t total = 0;
	char *p = buf;
	while (total < len) {
		ssize_t r = recv(fd, p + total, len - total, 0);
		if (r <= 0) {
			/* r == 0 signifie que le correspondant a fermé la connexion */
			if (r < 0 && errno == EINTR) continue;
			return -1;
		}
		total += (size_t)r;
	}
	return (ssize_t)total;
}

/**
 * @brief Récupère l'adresse IP locale de la machine.
 * * @param out    Buffer où stocker l'IP (ex: "192.168.1.15").
 * @param outlen Taille du buffer 'out'.
 */
int get_local_ip(char *out, size_t outlen) {
	char host[256];
	if (gethostname(host, sizeof(host)) != 0) return -1;

	struct addrinfo hints, *res, *p;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET; // On force IPv4 pour simplifier

	/* getaddrinfo permet de résoudre le nom d'hôte local en adresse IP */
	if (getaddrinfo(host, NULL, &hints, &res) != 0) return -1;
	
	for (p = res; p; p = p->ai_next) {
		struct sockaddr_in *sa = (struct sockaddr_in *)p->ai_addr;
		const char *addr = inet_ntoa(sa->sin_addr);
		if (addr) {
			strncpy(out, addr, outlen-1);
			out[outlen-1] = '\0';
			freeaddrinfo(res);
			return 0;
		}
	}
	freeaddrinfo(res);
	return -1;
}

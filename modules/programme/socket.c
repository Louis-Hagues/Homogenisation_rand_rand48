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

/* Utility: create server socket (bind+listen) */
int create_server_socket(int port, int backlog) {
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) { perror("socket"); return -1; }

	int opt = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

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

int accept_client(int server_sock) {
	struct sockaddr_in cli;
	socklen_t len = sizeof(cli);
	int c = accept(server_sock, (struct sockaddr*)&cli, &len);
	if (c < 0) { perror("accept"); return -1; }
	return c;
}

int create_client_and_connect(const char *server_ip, int port) 
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) { perror("socket"); return -1; }

	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
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


ssize_t send_all(int fd, const void *buf, size_t len) {
	size_t total = 0;
	const char *p = buf;
	while (total < len) {
		ssize_t s = send(fd, p + total, len - total, 0);
		if (s < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		total += (size_t)s;
	}
	return (ssize_t)total;
}

ssize_t recv_all(int fd, void *buf, size_t len) {
	size_t total = 0;
	char *p = buf;
	while (total < len) {
		ssize_t r = recv(fd, p + total, len - total, 0);
		if (r <= 0) {
			if (r < 0 && errno == EINTR) continue;
			return -1;
		}
		total += (size_t)r;
	}
	return (ssize_t)total;
}

int get_local_ip(char *out, size_t outlen) {
	char host[256];
	if (gethostname(host, sizeof(host)) != 0) return -1;
	struct addrinfo hints, *res, *p;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h> 


int create_socket(int domain, int type, int protocol) 
{
    int socket = socket(domain,type,protocol);

    if(socket = -1)
    {
        perror("Erreur lors de la création de l'objet socket");
    }
    int sockopt = setsockopt(socket,SOL_SOCKET,SO_REUSEADDR,)


}

int connect_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {}
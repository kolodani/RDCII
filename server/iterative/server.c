#include "server.h"
#include "utils.h"
#include "config.h"
#include "pi.h"
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

extern int server_socket;

int server_init(const char *ip, int port)
{
    struct sockaddr_in server_addr;

    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0)
    {
        fprintf(stderr, "Error creating socket\n");
        perror(NULL);
        return -1;
    }

    const int opt = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        fprintf(stderr, "Error configuring socket\n");
        perror(NULL);
        close(listen_socket);
        return -1;
    }

#ifdef SO_REUSEPORT
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        fprintf(stderr, "Error setting SO_REUSEPORT: \n");
        perror(NULL);
        close(listen_socket);
        return -1;
    }
#endif

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(listen_socket);
        return -1;
    }

    if (bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed: ");
        perror(NULL);
        close(listen_socket);
        return -1;
    }

    char ip_buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr.sin_addr, ip_buf, sizeof(ip_buf));
    printf("listening in %s:%d\n", ip_buf, port);

    if (listen(listen_socket, SOMAXCONN) < 0)
    {
        fprintf(stderr, "Listening error: \n");
        perror(NULL);
        close(listen_socket);
        return -1;
    }

    server_socket = listen_socket;
    return listen_socket;
}

int server_accept(int listen_socket, struct sockaddr_in *client_addr)
{

    socklen_t addrlen = sizeof(*client_addr);
    int new_socket = accept(listen_socket, (struct sockaddr *)client_addr, &addrlen);

    if (new_socket < 0 && errno != EINTR)
    {
        fprintf(stderr, "Error accepting connection: \n");
        perror(NULL);
        return -1;
    }

    return new_socket;
}

void server_loop(int socket)
{

    session_init(socket);

    if (welcome(current_sess) < 0)
        return;

    while (1)
    {
        if (getexe_command(current_sess) < 0)
            break;
    }

    session_cleanup();
}
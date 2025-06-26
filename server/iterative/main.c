#include "arguments.h"
#include "server.h"
#include "utils.h"
#include "signals.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    struct arguments args;

    if (parse_arguments(argc, argv, &args) != 0)
    {
        fprintf(stderr, "Error: invalid arguments.\n");
        return EXIT_FAILURE;
    }

    printf("Starting server on %s:%d\n", args.address, args.port);

    int master_socket = server_init(args.address, args.port);
    if (master_socket < 0)
    {
        fprintf(stderr, "Error: Could not start server.\n");
        return EXIT_FAILURE;
    }

    setup_signals();

    while (1)
    {
        struct sockaddr_in client_addr;
        int new_socket = server_accept(master_socket, &client_addr);
        if (new_socket < 0)
        {
            fprintf(stderr, "Error creating socket\n");
            perror(NULL);
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Connection from %s:%d accepted\n", client_ip, ntohs(client_addr.sin_port));

        server_loop(new_socket);

        printf("Connection from %s:%d closed\n", client_ip, ntohs(client_addr.sin_port));
    }

    close_fd(master_socket, "Master socket");

    return EXIT_SUCCESS;
}
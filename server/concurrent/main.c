#include "arguments.h"
#include "server.h"
#include "utils.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

int main(int argc, char **argv)
{
    struct arguments args;

    if (parse_arguments(argc, argv, &args) != 0)
    {
        return EXIT_FAILURE;
    }

    printf("Starting server on %s:%d\n", args.address, args.port);

    int listen_fd = server_init(args.address, args.port);
    if (listen_fd < 0)
    {
        return EXIT_FAILURE;
    }

    setup_signals();

    while (1)
    {
        struct sockaddr_in client_addr;

        memset(&client_addr, 0, sizeof(client_addr));
        int new_socket = server_accept(listen_fd, &client_addr);
        if (new_socket < 0)
        {
            continue;
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            close_fd(new_socket, "client (fork failed)");
            continue;
        }

        if (pid == 0)
        {
            pid_t pgid = getpgrp();
            if (setpgid(0, pgid) < 0)
            {
                perror("setpgid child");
            }
            printf("Child PID %d PGID %d\n", getpid(), getpgrp());

            setup_child_signals();

            close(listen_fd);

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            printf("[+] New connection from %s:%d handled by child PID %d\n", client_ip, ntohs(client_addr.sin_port), getpid());

            server_loop(new_socket);

            printf("[-] Child PID %d closing connection for %s:%d\n", getpid(), client_ip, ntohs(client_addr.sin_port));

            exit(EXIT_SUCCESS);
        }
        else
        {
            close_fd(new_socket, "client socket");
        }
    }

    close_fd(listen_fd, "listening socket");

    return EXIT_SUCCESS;
}

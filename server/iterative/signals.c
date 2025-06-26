#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "signals.h"
#include "session.h"
#include "utils.h"

int server_socket = -1;

static void handle_sigint(int sig)
{
    (void)sig;
    static volatile sig_atomic_t in_handler = 0;

    if (in_handler)
    {
        fprintf(stderr, "SIGINT handler reentered!\n");
        return;
    }
    in_handler = 1;

    static int sigint_count = 0;
    fprintf(stderr, "SIGINT handler called (count = %d) in PID %d\n", ++sigint_count, getpid());

    printf("[+] SIGINT received. Shutting down...\n");
    fflush(stdout);

    if (server_socket >= 0)
    {
        close_fd(server_socket, "listen socket");
        server_socket = -1;
    }

    sigset_t blockset, oldset;
    sigemptyset(&blockset);
    sigaddset(&blockset, SIGINT);
    if (sigprocmask(SIG_BLOCK, &blockset, &oldset) < 0)
    {
        perror("sigprocmask");
    }

    sigprocmask(SIG_SETMASK, &oldset, NULL);

    exit(EXIT_SUCCESS);
}

static void handle_sigterm(int sig)
{
    (void)sig;

    static volatile sig_atomic_t in_handler = 0;
    if (in_handler)
    {
        fprintf(stderr, "SIGTERM handler reentered!\n");
        return;
    }
    in_handler = 1;

    fprintf(stderr, "[+] SIGTERM received. Shutting down (PID %d)...\n", getpid());

    if (server_socket >= 0)
    {
        close_fd(server_socket, "listen socket");
        server_socket = -1;
    }

    exit(EXIT_SUCCESS);
}

void setup_signals(void)
{
    struct sigaction sa;

    printf("[DEBUG] Setting up signal handlers in PID %d\n", getpid());

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);

    sa.sa_flags = SA_RESTART;

    sa.sa_handler = handle_sigint;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }
    printf("[DEBUG] SIGINT handler installed in PID %d\n", getpid());

    sa.sa_handler = handle_sigterm;

    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }
}

void reset_signals(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

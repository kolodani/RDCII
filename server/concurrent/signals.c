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

    pid_t pgid = getpgrp();
    if (killpg(pgid, SIGTERM) < 0)
    {
        perror("killpg");
    }

    while (waitpid(-1, NULL, WNOHANG) > 0);

    sigprocmask(SIG_SETMASK, &oldset, NULL);

    exit(EXIT_SUCCESS);
}

static void handle_sigterm(int sig)
{
    (void)sig;

    static volatile sig_atomic_t in_handler = 0;
    if (in_handler)
    {
        fprintf(stderr, "SIGTERM handler reentered in parent!\n");
        return;
    }
    in_handler = 1;

    fprintf(stderr, "[+] SIGTERM received in parent. Shutting down (PID %d)...\n", getpid());

    if (server_socket >= 0)
    {
        close_fd(server_socket, "listen socket");
        server_socket = -1;
    }

    pid_t pgid = getpgrp();
    fprintf(stderr, "[DEBUG] Sending SIGTERM to (GROUP %d)...\n", (int)pgid);
    if (killpg(pgid, SIGTERM) < 0)
    {
        perror("killpg (parent)");
    }

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    exit(EXIT_SUCCESS);
}

static void handle_sigterm_child(int sig)
{
    (void)sig;

    printf("[*] Child PID %d received SIGTERM, cleaning up...\n", getpid());
    fflush(stdout);

    if (current_sess)
    {
        if (current_sess->control_sock >= 0)
        {
            close_fd(current_sess->control_sock, "control socket");
            current_sess->control_sock = -1;
        }
        if (current_sess->data_sock >= 0)
        {
            close_fd(current_sess->data_sock, "data socket");
            current_sess->data_sock = -1;
        }
    }

    exit(EXIT_SUCCESS);
}

void setup_signals(void)
{
    struct sigaction sa;

    if (setpgid(0, 0) < 0)
    {
        perror("setpgid parent");
        exit(EXIT_FAILURE);
    }

    printf("[DEBUG] Setting up signal handlers for parent in PID %d with PGID %d\n", getpid(), getpgrp());


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

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
}

void setup_child_signals(void)
{
    struct sigaction sa;

    printf("[DEBUG] Setting up signal handlers for child in PID %d\n", getpid());

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handle_sigterm_child;

    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction SIGTERM (child)");
        exit(EXIT_FAILURE);
    }
    printf("[DEBUG] SIGTERM handler for child installed in PID %d\n", getpid());

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction SIGINT (child)");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &sa, NULL);
}

void reset_signals(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
}

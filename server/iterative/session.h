#pragma once

#include "config.h"
#include <netinet/in.h>

typedef struct
{
    int control_sock;
    int data_sock;
    struct sockaddr_in data_addr;
    char current_user[USERNAME_MAX];
    uint8_t logged_in;
} ftp_session_t;

extern ftp_session_t *current_sess;

ftp_session_t *session_get(void);
void session_init(int control_fd);
void session_cleanup(void);

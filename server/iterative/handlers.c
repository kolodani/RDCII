#include "responses.h"
#include "pi.h"
#include "dtp.h"
#include "session.h"
#include "utils.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

void handle_USER(const char *args)
{
    ftp_session_t *sess = session_get();

    if (!args || strlen(args) == 0)
    {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    strncpy(sess->current_user, args, sizeof(sess->current_user) - 1);
    sess->current_user[sizeof(sess->current_user) - 1] = '\0';
    safe_dprintf(sess->control_sock, MSG_331);
}

void handle_PASS(const char *args)
{

    ftp_session_t *sess = session_get();

    if (sess->current_user[0] == '\0')
    {
        safe_dprintf(sess->control_sock, MSG_503);
        return;
    }

    if (!args || strlen(args) == 0)
    {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    if (check_credentials(sess->current_user, (char *)args) == 0)
    {
        sess->logged_in = 1;
        safe_dprintf(sess->control_sock, MSG_230);
    }
    else
    {
        safe_dprintf(sess->control_sock, MSG_530);
        sess->current_user[0] = '\0';
        sess->logged_in = 0;
    }
}

void handle_QUIT(const char *args)
{
    ftp_session_t *sess = session_get();
    (void)args;

    safe_dprintf(sess->control_sock, MSG_221);
    sess->current_user[0] = '\0';
    close_fd(sess->control_sock, "client socket");
    sess->control_sock = -1;
}

void handle_SYST(const char *args)
{
    ftp_session_t *sess = session_get();
    (void)args;

    safe_dprintf(sess->control_sock, MSG_215);
}

void handle_TYPE(const char *args)
{

    ftp_session_t *sess = session_get();

    if (!args || strlen(args) != 1) {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    if (args[0] == 'I')
    {
        safe_dprintf(sess->control_sock, MSG_203);
    }
    else
    {
        safe_dprintf(sess->control_sock, MSG_504);
    }
}

void handle_PORT(const char *args)
{
    ftp_session_t *sess = session_get();

    if (!args || strlen(args) == 0)
    {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    int h1, h2, h3, h4, p1, p2;
    if (sscanf(args, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6)
    {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    memset(&sess->data_addr, 0, sizeof(sess->data_addr));
    sess->data_addr.sin_family = AF_INET;
    sess->data_addr.sin_port = htons(p1 *256 + p2);

    char ip_str[INET_ADDRSTRLEN];
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", h1, h2, h3, h4);
    if (inet_pton(AF_INET, ip_str, &sess->data_addr.sin_addr) <= 0)
    {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    safe_dprintf(sess->control_sock, MSG_200);
}

void handle_RETR(const char *args)
{
    ftp_session_t *sess = session_get();

    if (!sess->logged_in)
    {
        safe_dprintf(sess->control_sock, MSG_530);
        return;
    }

    if (!args || strlen(args) == 0)
    {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    if (sess->data_addr.sin_port == 0)
    {
        safe_dprintf(sess->control_sock, MSG_503);
        return;
    }

    FILE *archivo = fopen(args, "rb");
    if (!archivo)
    {
        safe_dprintf(sess->control_sock, MSG_550, args);
        return;
    }

    int file_fd = open(args, O_RDONLY);
    if (file_fd < 0)
    {
        safe_dprintf(sess->control_sock, MSG_550, args);
        return;
    }

    int data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sock < 0)
    {
        safe_dprintf(sess->control_sock, MSG_425);
        close(file_fd);
        return;
    }

    if (connect(data_sock, (struct sockaddr *)&sess->data_addr, sizeof(sess->data_addr)) < 0)
    {
        safe_dprintf(sess->control_sock, MSG_425);
        fprintf(stderr, "Data connection failed:\n");
        perror(NULL);
        close(file_fd);
        close(data_sock);
        return;
    }

    safe_dprintf(sess->control_sock, MSG_150);

    char buf[BUFSIZE];
    ssize_t bytes;
    while ((bytes = read(file_fd, buf, sizeof(buf))) > 0)
    {
        if (send(data_sock, buf, bytes, 0) != bytes)
        {
            fprintf(stderr, "Partial Sent:\n");
            perror(NULL);
            break;
        }
    }

    close(file_fd);
    close(data_sock);

    safe_dprintf(sess->control_sock, MSG_226);
}

void handle_STOR(const char *args)
{
    ftp_session_t *sess = session_get();

    if (!sess->logged_in)
    {
        safe_dprintf(sess->control_sock, MSG_530);
        return;
    }

    if (!args || strlen(args) == 0)
    {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    if (sess->data_addr.sin_port == 0)
    {
        safe_dprintf(sess->control_sock, MSG_503);
        return;
    }

    int data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sock < 0)
    {
        safe_dprintf(sess->control_sock, MSG_425);
        return;
    }

    if (connect(data_sock, (struct sockaddr *)&sess->data_addr, sizeof(sess->data_addr)) < 0)
    {
        fprintf(stderr, "Data connection failed:\n");
        perror(NULL);
        safe_dprintf(sess->control_sock, MSG_425);
        close(data_sock);
        return;
    }

    int file_fd = open(args, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0)
    {
        safe_dprintf(sess->control_sock, MSG_550, args);
        close(data_sock);
        return;
    }

    safe_dprintf(sess->control_sock, MSG_150);

    char buf[BUFSIZE];
    ssize_t bytes;
    while ((bytes = recv(data_sock, buf, sizeof(buf), 0)) > 0)
    {
        if (write(file_fd, buf, bytes) != bytes)
        {
            fprintf(stderr, "Error writing to file:\n");
            perror(NULL);
            break;
        }
    }

    close(file_fd);
    close(data_sock);

    safe_dprintf(sess->control_sock, MSG_226);
}

void handle_NOOP(const char *args)
{
    ftp_session_t *sess = session_get();
    (void)args;
    (void)sess;

    safe_dprintf(sess->control_sock, MSG_200);
}

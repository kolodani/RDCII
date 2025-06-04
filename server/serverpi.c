#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "serverausftp.h"

#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"

// COMUNICACION

/*
 * recv_cmd Recepciona un comando desde el socket_descriptor
 * recv hace el receive / llamada definida en syssocket
 * strtok se comporta distinto a medida que va llamado / devuelve tokens,
 * la primera vez le pasas el buffer y el separador p indentificar tokens distintos
 * 
 * en operation obtengo la operacion comandos basicos de ftp- generalmente 4 letras, lo que mando el cliente
 */

int recv_cmd(int socket_descriptor, char *operation, char *param)
{
    char buffer[BUFSIZE];
    char *token;

    if (recv(socket_descriptor, buffer, BUFSIZE, 0) < 0)
    {
        fprintf(stderr, "error receiving data");
    }
    buffer[strcspn(buffer, "\r\n")] = 0;
    token = strtok(buffer, " ");
    if (token == NULL || strlen(token) < 4)
    {
        fprintf(stderr, "not valid ftp command");
        return 1;
    }
    else
    {
        strcpy(operation, token);
        token = strtok(NULL, " ");
        #if DEBUG
        printf("par %s\n", token);
        #endif
        if (token != NULL)
        {
            strcpy(param, token);
        }
    }
}
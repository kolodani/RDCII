#pragma once

#include "config.h"
#include <netinet/in.h>

struct arguments
{
    int port;
    int port_set;
    char address[INET_ADDRSTRLEN];
    int address_set;
};

int parse_arguments(int argc, char **argv, struct arguments *args);
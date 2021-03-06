#include <winsock2.h>
#include <stdio.h>
#include "server.h"

//TODO: Make these into logging messages that only act on files
void error_live(const char *s)
{
    fprintf(stderr, "Error: %s failed with error %d\n", s, WSAGetLastError());
    WSACleanup();
}

void error_die(const char *s)
{
    error_live(s);
    exit(EXIT_FAILURE);
}
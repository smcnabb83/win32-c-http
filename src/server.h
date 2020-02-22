#ifndef WIN32_C_HTTP_SERVER_H
#define WIN32_C_HTTP_SERVER_H

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define forever while(1)


typedef struct {
    u_short port;
} SERVER_CONFIG;

typedef struct {
    int  type;
    char *value;
    int length;
    char *cookie;
} REQUEST;

typedef struct {
    char *header;
    char *filename, *filepath;
    int  error;
} RESPONSE;

typedef RESPONSE* (*route_function)(REQUEST*);

#define REQUEST_SIZE 4096
#define DEFAULT_PORT 80

#undef DELETE
enum { RQ_UNDEF,GET,POST,PUT } response_types;

extern const char *DEFAULT_ERROR_404;

extern char *getcwd(char *, unsigned int);
extern char *get_header(RESPONSE *);
extern REQUEST *GetRequest(SOCKET);
extern void FreeRequest(REQUEST *);
extern RESPONSE *GetResponse(REQUEST *);
extern int SendResponse(SOCKET, RESPONSE *);
extern void error_live(const char *);
extern void error_die(const char *);

#endif
#ifndef WIN32_C_HTTP_SERVER_H
#define WIN32_C_HTTP_SERVER_H

#include"const.h"
#include<process.h>
#include<Windows.h>

#define forever while(1)

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

typedef struct {
    SOCKET msg_sock;
    SOCKADDR_IN client_addr;
    unsigned threadpool_id;
} SOCKMESSAGE;

#undef DELETE
enum { RQ_UNDEF,GET,POST,PUT } response_types;

unsigned __stdcall ProcessHTTPEvent(void *pArguments);


    extern const char *DEFAULT_ERROR_404;

extern char *getcwd(char *, unsigned int);
extern char *get_header(RESPONSE *);
extern REQUEST *GetRequest(SOCKET);
extern void FreeRequest(REQUEST *);
extern void FreeResponse(RESPONSE *);
extern RESPONSE *GetResponse(REQUEST *);
extern int SendResponse(SOCKET, RESPONSE *);
extern void error_live(const char *);
extern void error_die(const char *);

#endif
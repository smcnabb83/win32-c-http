#include <winsock2.h>
#include <stdio.h>
#include "server.h"
#include "globals.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

int get_request_type(char *buf)
{
    char retval[10] = {0};
    sscanf(buf, "%s ", &retval);

    if (!strcmp(retval, "GET"))
        return GET;
    else if (!strcmp(retval, "POST"))
        return POST;
    else if (!strcmp(retval, "PUT"))
        return PUT;
    else
        return RQ_UNDEF;
}

char *get_request_value(char *buf)
{
    char retval[100] = {0};

    sscanf(buf, "%s %s ", &retval, &retval);  // tee hee

    //TODO: Do we automatically want to assume that all requests to the base go to index.html?
    if (retval[strlen(retval)-1] == '/')
        strcat(retval, "index.html");

    return strdup(retval);
}

char* get_request_cookie(char *buf, int len)
{
    //TODO: Very inefficient - make this run better
    char* bufpos = buf;
    char *result = malloc(200);
    char *returnResult = malloc(200);
    for(int i = 0; i < len; i++){

        int res = sscanf(bufpos, "%s", result);
        if(res == EOF){
            return 0;
        }
        if(strcmp(result, "Cookie:") == 0){
            sscanf(bufpos, "%s %s", result, returnResult);
            break;
        }
        bufpos++;
    }
    free(result);
    return returnResult;
}

void FreeRequest(REQUEST* req){
    free(req->cookie);
    free(req->value);
    free(req);
}

REQUEST *GetRequest(SOCKET sock)
{
    REQUEST *request;
    int msg_len;
    char buf[REQUEST_SIZE];

    msg_len = recv(sock, buf, sizeof(buf), 0);
    char* cookie = get_request_cookie(buf, msg_len);

    request         = malloc(sizeof(REQUEST));
    request->type   = get_request_type(buf);
    request->value  = get_request_value(buf);
    request->length = msg_len;
    request->cookie = cookie;

    return request;
}
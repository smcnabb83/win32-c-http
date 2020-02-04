#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "server.h"

int thread_result_status[MAX_CONNECTIONS];

unsigned __stdcall ProcessHTTPEvent(void* pArguments){

    SOCKMESSAGE* msg = (SOCKMESSAGE*)(pArguments);

    printf("\n\n#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$\n\n");
    printf("Connected to %s:%d\n", inet_ntoa(msg->client_addr.sin_addr), htons(msg->client_addr.sin_port));

    REQUEST *request = GetRequest(msg->msg_sock);
    printf("Client requested %d %s\n", request->type, request->value);

    if (request->length == 0)
    {
        FreeRequest(request);
        _endthreadex(0);
        return 0;
    }

    RESPONSE *response = GetResponse(request);
    int sent = SendResponse(msg->msg_sock, response);

    closesocket(msg->msg_sock);
    FreeRequest(request);
    FreeResponse(response);

    if (sent == 0)
    {
        thread_result_status[msg->threadpool_id] = 1;
        _endthreadex(0);
        return 0;
    }

    if (sent == 1)
    {
        //TODO - emulate behavior of goto listen_goto
        thread_result_status[msg->threadpool_id] = 2;
        _endthreadex(0);
        return 0;
    }

    thread_result_status[msg->threadpool_id] = 1;
    _endthreadex(0);
    return 0;
}

int main(int argc, char **argv)
{
    //set up stuff for multiple threads
    HANDLE* handles[MAX_CONNECTIONS];
    unsigned threadids[MAX_CONNECTIONS];
    SOCKMESSAGE* socketMessages[MAX_CONNECTIONS];

    for(int i = 0; i < MAX_CONNECTIONS; i++){
        handles[i] = NULL;
        threadids[i] = 0;
        socketMessages[i] = malloc(sizeof(SOCKMESSAGE));
        thread_result_status[i] = 0;
    }

    int addr_len;
    struct sockaddr_in local, client_addr;

    SOCKET sock, msg_sock;
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
        error_die("WSAStartup()");

    // Fill in the address structure
    local.sin_family        = AF_INET;
    local.sin_addr.s_addr   = INADDR_ANY;
    local.sin_port          = htons(DEFAULT_PORT);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET)
        error_die("socket()");

    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
        error_die("bind()");

listen_goto:

    if (listen(sock, 5) == SOCKET_ERROR)
        error_die("listen()");

    printf("Waiting for connection...\n");

    forever
    {
        addr_len = sizeof(client_addr);
        msg_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);
        if (msg_sock == INVALID_SOCKET || msg_sock == -1)
            error_die("accept()");

        for(int i = 0; i < MAX_CONNECTIONS; i++){
            if(handles[i] == NULL){
                socketMessages[i]->msg_sock = msg_sock;
                socketMessages[i]->client_addr = client_addr;
                socketMessages[i]->threadpool_id = i;
                handles[i] = (HANDLE)_beginthreadex(NULL, 0, &ProcessHTTPEvent, (void *)socketMessages[i], 0, &threadids[i]);
                //Break out to stop initializing multiple threads for the same socketmessage
                break;
            }
        }

        // detect finished threads and prep them for new users
        for(int i = 0; i < MAX_CONNECTIONS; i++){
            if(thread_result_status[i] > 0){
                int resTemp = thread_result_status[i];
                thread_result_status[i] = 0;
                handles[i] = NULL;
                threadids[i] = 0;
                if(resTemp == 2){
                    goto listen_goto;
                }
            }
        }
    }

    WSACleanup();
}
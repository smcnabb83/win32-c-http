
#include "server.h"
#include "ringbuf.h"
DWORD WINAPI processNextSocketInQueue(LPVOID args)
{
    //TODO: Right now, we're spinning forever. Maybe we can clean this up so we're not spinning multiple threads forever. 
    forever
    {
        int sockID = nextSocketToRead;
        if (sockID != nextSocketToWrite)
        {
            if (sockID == InterlockedCompareExchange(&nextSocketToRead, ((sockID + 1) % MAX_SOCKETS), sockID))
            {
                if (SocketsToProcess[sockID].conn == INVALID_SOCKET || SocketsToProcess[sockID].conn == -1){
                    //TODO: Log this as an error, but continue in case the socket being invalid was a 1 time thing
                    printf("We have an invalid socket for some reason. SocketID: %d\n", sockID);
                    continue;
                }

                printf("\n\n#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$#$\n\n");
                printf("Connected to %s:%d\n", inet_ntoa(SocketsToProcess[sockID].client_addr.sin_addr), htons(SocketsToProcess[sockID].client_addr.sin_port));

                REQUEST *request = GetRequest(SocketsToProcess[sockID].conn);
                printf("Client requested %d %s\n", request->type, request->value);

                if (request->length == 0)
                {
                    printf("Request length 0\n");
                    FreeRequest(request);

                    continue;
                }

                RESPONSE *response = GetResponse(request);
                int sent = SendResponse(SocketsToProcess[sockID].conn, response);

                printf("response sent\n");

                if (sent == 0)
                {
                    globalErrorCon = ERRCON_TERMINATE_SERVER;
                }
                else if (sent == -1)
                {
                    globalErrorCon = ERRCON_RESET_LISTENER;
                }

                closesocket(SocketsToProcess[sockID].conn);
                FreeRequest(request);
            }
        }
    }
}

int main(int argc, char **argv)
{
    int addr_len;
    struct sockaddr_in local, client_addr;

    SOCKET sock;
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

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int threadsToUse = ((int)info.dwNumberOfProcessors) - 1;
    printf("Got System Info]n");

    HANDLE *threads = malloc(sizeof(HANDLE)*threadsToUse);
    printf("Allocated thread memory space\n");

    HANDLE *currentThread = threads;
    //TODO: Do we really need bsArgumentToPass
    int bsArgumentToPass = 0;
    for(int i = 0; i < threadsToUse; i++){
        printf("allocating thread %d of %d", i, threadsToUse);
        *currentThread = CreateThread(NULL, 0, processNextSocketInQueue, &bsArgumentToPass, 0, NULL);
        currentThread++;
    }
    int count = 0;

    forever
    {
        addr_len = sizeof(client_addr);
        SocketsToProcess[nextSocketToWrite].conn = accept(sock, (struct sockaddr*)&client_addr, &addr_len);
        SocketsToProcess[nextSocketToWrite].client_addr = client_addr;
        SocketsToProcess[nextSocketToWrite].address_length = addr_len;
        while((nextSocketToWrite + 1) % MAX_SOCKETS == nextSocketToRead); //spin until read cursor moves to the next
        nextSocketToWrite = ((nextSocketToWrite + 1) % MAX_SOCKETS);        

        if (globalErrorCon == ERRCON_TERMINATE_SERVER)
            break;
        else if (globalErrorCon == ERRCON_RESET_LISTENER)
            goto listen_goto;

    }

    WSACleanup();
}


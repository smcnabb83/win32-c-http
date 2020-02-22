
#include "server.h"
#include "ringbuf.h"
#include "globals.h"

void add_route(SERVER_CONFIG config, char* route_to_match, route_function function){
    //TODO: actually implement this!
}

route_function match_route(REQUEST* request){
    return (route_function)GetResponse;
}

RESPONSE* route_request(REQUEST* request){
    route_function function = match_route(request);
    return function(request);
}

DWORD WINAPI processNextSocketInQueue(LPVOID args)
{
    RING_BUFFER* ringBuffer = (RING_BUFFER *)args;
    forever
    {
        int sockID = ringBuffer->nextSocketToRead;
        if (sockID != ringBuffer->nextSocketToWrite)
        {
            if (sockID == InterlockedCompareExchange(&ringBuffer->nextSocketToRead, ((sockID + 1) % MAX_SOCKETS), sockID))
            {
                DEBUGPRINT("socket buffer info: %d\n ", (int)ringBuffer->SocketBuffer[sockID].conn);
                if (ringBuffer->SocketBuffer[sockID].conn == INVALID_SOCKET || ringBuffer->SocketBuffer[sockID].conn == -1){
                    //TODO: Log this as an error, but continue in case the socket being invalid was a 1 time thing
                    DEBUGPRINT("We have an invalid socket for some reason. SocketID: %d\n", sockID);
                    break;
                }

                REQUEST *request = GetRequest(ringBuffer->SocketBuffer[sockID].conn);
                if (request->length == 0)
                {
                    DEBUGPRINT("Request length 0\n");
                    FreeRequest(request);

                    continue;
                }
                RESPONSE *response = route_request(request);
                //RESPONSE *response = GetResponse(request);
                int sent = SendResponse(ringBuffer->SocketBuffer[sockID].conn, response);


                if (sent == 0)
                {
                    ringBuffer->bufferErrorCon = ERRCON_TERMINATE_SERVER;
                }
                else if (sent == -1)
                {
                    ringBuffer->bufferErrorCon = ERRCON_RESET_LISTENER;
                }

                closesocket(ringBuffer->SocketBuffer[sockID].conn);
                FreeRequest(request);
            }
        } else {
            WaitForSingleObjectEx(ringBuffer->RingBufferSemaphore, INFINITE, FALSE);
        }
    }
    return (DWORD) 1;
}

void PutNextMessageInQueue(RING_BUFFER* ringBuffer, SOCKET sock, SOCKADDR_IN* client_addr){
    int addr_len = sizeof(*client_addr);
    ringBuffer->SocketBuffer[ringBuffer->nextSocketToWrite].client_addr = *client_addr;
    ringBuffer->SocketBuffer[ringBuffer->nextSocketToWrite].address_length = addr_len;
    ringBuffer->SocketBuffer[ringBuffer->nextSocketToWrite].conn = accept(sock, (SOCKADDR *)&client_addr, &addr_len);
    if(ringBuffer->SocketBuffer[ringBuffer->nextSocketToWrite].conn == INVALID_SOCKET){
        DEBUGPRINT("Socket accept failed for queue slot %d, with an error code of %d\n", ringBuffer->nextSocketToWrite, WSAGetLastError());
        
    }
}

SERVER_CONFIG create_server_config(){
    SERVER_CONFIG config;
    config.port = 80;
    return config;
}

int serve(SERVER_CONFIG config){
    //Initialization
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int threadsToUse = ((int)info.dwNumberOfProcessors) - 1;

    RING_BUFFER ringBuffer = GetNewRingBuffer(threadsToUse);

    struct sockaddr_in local, client_addr;

    SOCKET sock;
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
        error_die("WSAStartup()");

    // Fill in the address structure
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    DEBUGPRINT("port configured: %d", config.port);
    local.sin_port = htons(DEFAULT_PORT);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET)
        error_die("socket()");

    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
        error_die("bind()");

    HANDLE *threads = malloc(sizeof(HANDLE) * threadsToUse);
    RING_BUFFER *ringBufferPointer = &ringBuffer;
    for (int i = 0; i < threadsToUse; i++)
    {
        DEBUGPRINT("allocating thread %d of %d\n", i, threadsToUse);
        threads[i] = CreateThread(NULL, 0, processNextSocketInQueue, &ringBuffer, 0, NULL);
    }
    int count = 0;

listen_goto:

    if (listen(sock, 5) == SOCKET_ERROR)
        error_die("listen()");

    DEBUGPRINT("Waiting for connection...\n");

    //Start our message loop
    forever
    {
        PutNextMessageInQueue(&ringBuffer, sock, &client_addr);
        while (!RingBufferCanWrite(&ringBuffer)); //spin until read cursor moves to the next
        ringBuffer.nextSocketToWrite = ((ringBuffer.nextSocketToWrite + 1) % MAX_SOCKETS);
        ReleaseSemaphore(ringBuffer.RingBufferSemaphore, 1, 0);

        if (ringBuffer.bufferErrorCon == ERRCON_TERMINATE_SERVER)
            break;
        else if (ringBuffer.bufferErrorCon == ERRCON_RESET_LISTENER)
            goto listen_goto;
    }

    free(threads);
    WSACleanup();
    return 0;
}

int main(int argc, char **argv)
{
    SERVER_CONFIG config = create_server_config();
    add_route(config, "/", &GetResponse);
    serve(config);
    return 0;   
}


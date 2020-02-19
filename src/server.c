
#include "server.h"
#include "ringbuf.h"
#include "globals.h"

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
                if (ringBuffer->SocketBuffer[sockID].conn == INVALID_SOCKET || ringBuffer->SocketBuffer[sockID].conn == -1){
                    //TODO: Log this as an error, but continue in case the socket being invalid was a 1 time thing
                    DEBUGPRINT("We have an invalid socket for some reason. SocketID: %d\n", sockID);
                    continue;
                }

                REQUEST *request = GetRequest(ringBuffer->SocketBuffer[sockID].conn);
                if (request->length == 0)
                {
                    DEBUGPRINT("Request length 0\n");
                    FreeRequest(request);

                    continue;
                }

                RESPONSE *response = GetResponse(request);
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
}

void PutNextMessageInQueue(RING_BUFFER* ringBuffer, SOCKET sock, struct sockaddr_in* client_addr){
    SOCKET ret;
    int addr_len = sizeof(client_addr);
    ringBuffer->SocketBuffer[ringBuffer->nextSocketToWrite].conn = accept(sock, (struct sockaddr *)&client_addr, &addr_len);
    ringBuffer->SocketBuffer[ringBuffer->nextSocketToWrite].client_addr = *client_addr;
    ringBuffer->SocketBuffer[ringBuffer->nextSocketToWrite].address_length = addr_len;
}

int serve(){
    //Initialization
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int threadsToUse = ((int)info.dwNumberOfProcessors) - 1;

    RING_BUFFER ringBuffer = GetNewRingBuffer(threadsToUse);

    int addr_len;
    struct sockaddr_in local, client_addr;

    SOCKET sock;
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
        error_die("WSAStartup()");

    // Fill in the address structure
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
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

    WSACleanup();
}

int main(int argc, char **argv)
{
    serve();
    return 0;
   
}


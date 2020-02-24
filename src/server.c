
#include "server.h"
#include "ringbuf.h"
#include "globals.h"

void add_route(SERVER_CONFIG* config, char* route_to_match, route_function function){
    ROUTE_INFO** current_route_info = &config->route_info_nodes;
    while(*current_route_info){
        current_route_info = (ROUTE_INFO**) &((*current_route_info)->next);
    }
    (*current_route_info) = (ROUTE_INFO*)malloc(sizeof(ROUTE_INFO));
    (*current_route_info)->route = route_to_match;
    (*current_route_info)->function = function;
    (*current_route_info)->next = NULL;
}

//matches parts of routes that are between \ and 
//TODO: This doesn't work quite right - fix. 
BOOL match_chunk(char* left, char* right, int* advanceCount){
    BOOL matched = FALSE;
    while(matched == FALSE && *left != '\0' && *right != '\0'){
        if(*left != *right){
            break;
        }
        if(*left == '/' && *right == '/'){
            matched = TRUE;
        }
        left++;
        right++;
        (*advanceCount)++;
    }

    if(*left == *right){
        matched = TRUE;
    }

    return matched;
}

//TODO: This is still fragile & finicky - need to make this more robust later.
route_function match_route(SERVER_CONFIG config, REQUEST* request){
    int matchScore = 0;
    route_function func = NULL;
    ROUTE_INFO* info = config.route_info_nodes;
    DEBUGPRINT("Route Info at start %s\n", info->route);
    while(info){
        DEBUGPRINT("Matching route %s with path %s --- ", info->route, request->value);
        char* reqIter = request->value;
        char* matchIter = info->route;
        int tempScore = 0;
        BOOL match = FALSE;
        //TODO: using advanceCount seems kinda janky - is there a better way to do this?
        int advanceCount = 0;
        while(match_chunk(reqIter, matchIter, &advanceCount)){
            tempScore++;
            reqIter += advanceCount;
            matchIter += advanceCount;
            advanceCount = 0;
        }

        if(*matchIter == '*' || (*matchIter == *reqIter)){
            match = TRUE;
            if(*matchIter == *reqIter){
                tempScore++;
            }
        }

        if (match && tempScore > matchScore)
        {
            matchScore = tempScore;
            func = info->function;
        }
        if(match){
            DEBUGPRINT("Route matched with a score of %d, max score of %d\n", tempScore, matchScore);
        }
        else {
            DEBUGPRINT("Route did not match");
        }
        info = (ROUTE_INFO *)info->next;
    }
    return func;
}

int route_request(SERVER_CONFIG config, SOCKET sock, REQUEST* request){
    route_function function = match_route(config, request);
    if(function == NULL){
        return Send404Error(sock);
    }
    return function(sock, request);
}

DWORD WINAPI processNextSocketInQueue(LPVOID args)
{
    SERVER_CONFIG* config = (SERVER_CONFIG*) args;
    RING_BUFFER* ringBuffer = &config->ringBuffer;
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
                int sent = route_request(*config, ringBuffer->SocketBuffer[sockID].conn, request);
                //RESPONSE *response = GetResponse(request);


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

    ConfigRingBuffer(threadsToUse, &config.ringBuffer);
    RING_BUFFER* ringBuffer = &config.ringBuffer;

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
    RING_BUFFER *ringBufferPointer = ringBuffer;
    for (int i = 0; i < threadsToUse; i++)
    {
        DEBUGPRINT("allocating thread %d of %d\n", i, threadsToUse);
        threads[i] = CreateThread(NULL, 0, processNextSocketInQueue, &config, 0, NULL);
    }
    int count = 0;

listen_goto:

    if (listen(sock, 5) == SOCKET_ERROR)
        error_die("listen()");

    DEBUGPRINT("Waiting for connection...\n");

    //Start our message loop
    forever
    {
        PutNextMessageInQueue(ringBuffer, sock, &client_addr);
        while (!RingBufferCanWrite(ringBuffer)); //spin until read cursor moves to the next
        ringBuffer->nextSocketToWrite = ((ringBuffer->nextSocketToWrite + 1) % MAX_SOCKETS);
        ReleaseSemaphore(ringBuffer->RingBufferSemaphore, 1, 0);

        if (ringBuffer->bufferErrorCon == ERRCON_TERMINATE_SERVER)
            break;
        else if (ringBuffer->bufferErrorCon == ERRCON_RESET_LISTENER)
            goto listen_goto;
    }

    free(threads);
    WSACleanup();
    return 0;
}

#ifndef WIN32_C_HTTP_TEST_BUILD
int main(int argc, char **argv)
{
    SERVER_CONFIG config = create_server_config();
    add_route(&config, "/*", &ProcessFileRequest);
    add_route(&config, "/date/", &SendDateResponse);
    serve(config);
    return 0;   
}
#endif


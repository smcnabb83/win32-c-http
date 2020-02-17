#include<winsock2.h>

enum ErrorCondition
{
  ERRCON_NONE,
  ERRCON_RESET_LISTENER,
  ERRCON_TERMINATE_SERVER
};

#define MAX_SOCKETS 255
typedef struct
{
  SOCKET conn;
  struct sockaddr_in client_addr;
  int address_length;
} SOCKET_INFO;

SOCKET_INFO SocketsToProcess[MAX_SOCKETS];
volatile long nextSocketToRead = 0;
int nextSocketToWrite = 0;
int resetListener = 0;
int globalErrorCon = ERRCON_NONE;

#include<winsock2.h>

enum ErrorCondition
{
  ERRCON_NONE,
  ERRCON_RESET_LISTENER,
  ERRCON_TERMINATE_SERVER
};

#define MAX_SOCKETS 2048
typedef struct
{
  SOCKET conn;
  SOCKADDR_IN client_addr;
  int address_length;
} SOCKET_INFO;

typedef struct{
  SOCKET_INFO SocketBuffer[MAX_SOCKETS];
  HANDLE RingBufferSemaphore;
  volatile long nextSocketToRead;
  int nextSocketToWrite;
  int bufferErrorCon;
} RING_BUFFER;

RING_BUFFER GetNewRingBuffer(int ThreadCount);
BOOL RingBufferCanWrite(RING_BUFFER* buffer);

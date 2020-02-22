#include"ringbuf.h"

RING_BUFFER GetNewRingBuffer(int threadCount){
  RING_BUFFER buffer;
  buffer.RingBufferSemaphore = CreateSemaphoreEx(NULL, 0, threadCount, NULL, 0, SEMAPHORE_ALL_ACCESS);
  buffer.bufferErrorCon = ERRCON_NONE;
  buffer.nextSocketToRead = 0;
  buffer.nextSocketToWrite = 0;
  return buffer;
}

void ConfigRingBuffer(int threadCount, RING_BUFFER *buffer)
{
  buffer->RingBufferSemaphore = CreateSemaphoreEx(NULL, 0, threadCount, NULL, 0, SEMAPHORE_ALL_ACCESS);
  buffer->bufferErrorCon = ERRCON_NONE;
  buffer->nextSocketToRead = 0;
  buffer->nextSocketToWrite = 0;
}

BOOL RingBufferCanWrite(RING_BUFFER *buffer){
  BOOL ret = (buffer->nextSocketToWrite + 1) % MAX_SOCKETS != buffer->nextSocketToRead;
  return ret;
}
/* Includes ------------------------------------------------------------------*/
#include <assert.h>
#include "ring_buffer.h"


bool RingBuffer_Init(RingBuffer *ringBuffer, char *dataBuffer, size_t dataBufferSize) 
{
	assert(ringBuffer);
	assert(dataBuffer);
	assert(dataBufferSize > 0);
	
	if ((ringBuffer) && (dataBuffer) && (dataBufferSize > 0)) {
	  //TODO

    ringBuffer->buf = dataBuffer;
	  ringBuffer->size = dataBufferSize;
    ringBuffer->head = 0;
    ringBuffer->tail = 0;
    ringBuffer->count = 0;
	  
	  return true;
	}
	
	return false;
}

bool RingBuffer_Clear(RingBuffer *ringBuffer)
{
	assert(ringBuffer);
	
	if (ringBuffer) {
		//TODO
    ringBuffer->head = 0;
    ringBuffer->tail = 0;
    ringBuffer->count = 0;

    return true;
	}
	return false;
}

bool RingBuffer_IsEmpty(const RingBuffer *ringBuffer)
{
  assert(ringBuffer);	
	//TODO

  if(ringBuffer->count != 0){
    return false;
  }
	
	return true;
}

size_t RingBuffer_GetLen(const RingBuffer *ringBuffer)
{
	assert(ringBuffer);
	
	if (ringBuffer) {
		// TODO
    return ringBuffer->count;
	}
	return 0;
	
}

size_t RingBuffer_GetCapacity(const RingBuffer *ringBuffer)
{
	assert(ringBuffer);
	
	if (ringBuffer) {
		// TODO
    return ringBuffer->size;
	}
	return 0;	
}


bool RingBuffer_PutChar(RingBuffer *ringBuffer, char c)
{
	assert(ringBuffer);
	
	if (ringBuffer) {
		//TODO    

    if(ringBuffer->count < ringBuffer->size){
		  ringBuffer->buf[ringBuffer->head] = c; //wpisywanie znaku

      ringBuffer->head = (ringBuffer->head +1) % ringBuffer->size; //zwiekszenie head i jezeli head dojdzie do konca to sie zawija od poczatku
      
      ringBuffer->count++; //zwiekszenie liczby elementów
		  return true;
	  }
  }
	return false;
}

bool RingBuffer_GetChar(RingBuffer *ringBuffer, char *c)
{
	assert(ringBuffer);
	assert(c);
	
  if ((ringBuffer) && (c)) {
		//TODO

    if(ringBuffer->count>0){
      *c = ringBuffer->buf[ringBuffer->tail]; //odczytanie znaku

      ringBuffer->tail = (ringBuffer->tail +1) % ringBuffer->size; //zwiekszenie tail i jezeli tail dojdzie do konca to sie zawija od poczatku
      
      ringBuffer->count--; //zmniejszenie liczby elementów
      return true;
    }
    

	}
	return false;
}

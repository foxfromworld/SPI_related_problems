#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct CircularBuffer CircularBuffer;

struct CircularBuffer{
	int *buffer;
	int head;
	int tail;
	int length;
	bool full;
	int (*read)(CircularBuffer *cb);
	void (*write)(CircularBuffer *cb, int value);
	pthread_mutex_t mutex;
};

int readCircularBuffer(struct CircularBuffer *cb);
void writeCircularBuffer(struct CircularBuffer *cb, int value);
bool isEmpty(CircularBuffer *cb);

CircularBuffer* createBuffer(int size){
	CircularBuffer *cb = (CircularBuffer*)malloc(sizeof(CircularBuffer));
	cb->buffer = (int*)malloc(sizeof(int)*size);	
	cb->length =  size;
	cb->head = 0;
	cb->tail = 0;	
	cb->full = false;
	cb->read = readCircularBuffer;
	cb->write = writeCircularBuffer;
	pthread_mutex_init(&cb->mutex, NULL);
	return cb;
}
void writeCircularBuffer(CircularBuffer *cb, int value){
	pthread_mutex_lock(&cb->mutex);
	cb->buffer[cb->head] = value;
	if(cb->full){
		cb->tail = (cb->tail+1)%cb->length;
	}
	cb->head = (cb->head+1)%cb->length;
	cb->full = (cb->head==cb->tail);
	pthread_mutex_unlock(&cb->mutex);
}
int readCircularBuffer(CircularBuffer *cb){
	pthread_mutex_lock(&cb->mutex);
	if(isEmpty(cb)){
		pthread_mutex_unlock(&cb->mutex);
		return -1;
	}
	int value = cb->buffer[cb->tail];
	cb->tail = (cb->tail+1)%cb->length;
	cb->full = false;
	pthread_mutex_unlock(&cb->mutex);
	return value;
}

bool isFull(CircularBuffer *cb){
	pthread_mutex_lock(&cb->mutex);
	bool full = cb->full;
	pthread_mutex_unlock(&cb->mutex);	
	return full;
}

bool isEmpty(CircularBuffer *cb){
	pthread_mutex_lock(&cb->mutex);
	bool empty = ((!cb->full)&&(cb->tail==cb->head));
	pthread_mutex_unlock(&cb->mutex);	
	return empty;
}

void freeBuffer(CircularBuffer *cb){
	free(cb->buffer);
	pthread_mutex_destroy(&cb->mutex);	
	free(cb);
}

int main(){
	CircularBuffer *spi_rx_buffer = createBuffer(8);
	int data_stream[] = {10, 20, 30, 40, 50, 60, 70, 80, 90};
	size_t data_length = sizeof(data_stream)/sizeof(data_stream[0]);
	for(size_t i = 0; i<data_length; i++){
		spi_rx_buffer->write(spi_rx_buffer , data_stream[i]);
	}
	while(!isEmpty(spi_rx_buffer)){
		int value = spi_rx_buffer ->read(spi_rx_buffer );
		if(value != -1)
			printf("Data: %d\n", value);
	}
	freeBuffer(spi_rx_buffer);
	return 0;
}
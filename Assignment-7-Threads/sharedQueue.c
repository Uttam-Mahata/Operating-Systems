/* 
Implementation of a queue to be used by multiple threads of a process. 
* Producer Consumer Problem
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


#define MAX_QUEUE_SIZE 10
#define MAX_PRODUCER_THREAD 10
#define MAX_CONSUMER_THREAD 10
#define MAX_SLEEP_TIME 5

typedef struct {
    int data[MAX_QUEUE_SIZE];
    int front, rear;
    pthread_mutex_t mutex;
    pthread_cond_t full, empty;
} Queue;

Queue queue;

void init(Queue *q) {
    q->front = q->rear = -1;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->full, NULL);
    pthread_cond_init(&q->empty, NULL);
}

void enqueue(Queue *q, int data) {
    pthread_mutex_lock(&q->mutex);
    if ((q->rear + 1) % MAX_QUEUE_SIZE == q->front) {
	pthread_cond_wait(&q->full, &q->mutex);
    }
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    q->data[q->rear] = data;
    if (q->front == -1) {
	q->front = q->rear;
    }
    pthread_cond_signal(&q->empty);
    pthread_mutex_unlock(&q->mutex);
}

int dequeue(Queue *q) {
    int data;
    pthread_mutex_lock(&q->mutex);
    if (q->front == -1) {
	pthread_cond_wait(&q->empty, &q->mutex);
    }
    data = q->data[q->front];
    if (q->front == q->rear) {
	q->front = q->rear = -1;
    } else {
	q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    }
    pthread_cond_signal(&q->full);
    pthread_mutex_unlock(&q->mutex);
    return data;
}

void *producer(void *arg) {
    int data;
    while (1) {
	data = rand() % 100;
	enqueue(&queue, data);
	printf("Data Produced: %d\n", data);
	sleep(rand() % MAX_SLEEP_TIME);
    }
}

void *consumer(void *arg) {
    int data;
    while (1) {
	data = dequeue(&queue);
	printf("Data Consumed: %d\n", data);
	sleep(rand() % MAX_SLEEP_TIME);
    }
}

int main() {
    pthread_t producer_thread[MAX_PRODUCER_THREAD], consumer_thread[MAX_CONSUMER_THREAD];
    init(&queue);
    for (int i = 0; i < MAX_PRODUCER_THREAD; i++) {
	pthread_create(&producer_thread[i], NULL, producer, NULL);
    }
    for (int i = 0; i < MAX_CONSUMER_THREAD; i++) {
	pthread_create(&consumer_thread[i], NULL, consumer, NULL);
    }
    for (int i = 0; i < MAX_PRODUCER_THREAD; i++) {
	pthread_join(producer_thread[i], NULL);
    }
    for (int i = 0; i < MAX_CONSUMER_THREAD; i++) {
	pthread_join(consumer_thread[i], NULL);
    }
    return 0;
}



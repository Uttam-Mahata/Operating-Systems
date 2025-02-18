#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define QUEUE_SIZE 5


/* Structure to represent the queue. */
typedef struct {
    int data[QUEUE_SIZE];
    int front;
    int rear;
    pthread_mutex_t lock;
    pthread_cond_t notEmpty; // Condition variable to wait on when the queue is empty.
    pthread_cond_t notFull; // Condition variable to wait on when the queue is full.
} Queue;

Queue queue;

/* Initialize the queue. */
void initQueue(Queue *queue) {
    queue->front = 0;
    queue->rear = 0;
    /* Initialize the mutex and condition variables. */
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->notEmpty, NULL);
    pthread_cond_init(&queue->notFull, NULL);
}
/* Add an element to the queue. */
void enqueue(Queue *queue, int data) {
    pthread_mutex_lock(&queue->lock);
    /* Wait while the queue is full. */
    while ((queue->rear + 1) % QUEUE_SIZE == queue->front) {
	pthread_cond_wait(&queue->notFull, &queue->lock);
    }
    /* Add the data to the queue. */       
    queue->data[queue->rear] = data;
    queue->rear = (queue->rear + 1) % QUEUE_SIZE;
    /* Signal that the queue is not empty. */
    pthread_cond_signal(&queue->notEmpty);
    /* Unlock the mutex. */
    pthread_mutex_unlock(&queue->lock);
}
/* Remove an element from the queue. */
int dequeue(Queue *queue) {
    int data;
    pthread_mutex_lock(&queue->lock);
    while (queue->front == queue->rear) {
	pthread_cond_wait(&queue->notEmpty, &queue->lock);
    }
    data = queue->data[queue->front];
    queue->front = (queue->front + 1) % QUEUE_SIZE;
    pthread_cond_signal(&queue->notFull);
    pthread_mutex_unlock(&queue->lock);
    return data;
}
/* Producer thread function. */
void *producer(void *arg) {
    int i;
    for (i = 0; i < 10; i++) {
	enqueue(&queue, i);
	printf("Produced: %d\n", i);
	sleep(1);
    }
    return NULL;
}
/* Consumer thread function. */
void *consumer(void *arg) {
    int i;
    for (i = 0; i < 10; i++) {
	printf("Consumed: %d\n", dequeue(&queue));
	sleep(2);
    }
    return NULL;
}

int main() {
	/* Create two threads, one for the producer and one for the consumer. */

    pthread_t producerThread, consumerThread;
    /* Initialize the queue and create the threads. */
    initQueue(&queue);
    /* Create the threads. */
    pthread_create(&producerThread, NULL, producer, NULL);
    pthread_create(&consumerThread, NULL, consumer, NULL);
    /* Wait for the threads to finish. */
    pthread_join(producerThread, NULL);
    pthread_join(consumerThread, NULL);
    return 0;
}

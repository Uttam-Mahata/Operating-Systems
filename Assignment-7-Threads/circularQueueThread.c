/* Implementation of a circular queue to be used by multiple threads of a process.
 * The queue is implemented using a fixed size array and a mutex lock to ensure
 * thread safety. The queue is implemented as a circular queue to avoid the need
 * to shift elements when adding or removing elements from the queue.
 */

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
    pthread_cond_t notEmpty;
    pthread_cond_t notFull;
} Queue;

Queue queue;

/* Initialize the queue. */
void initQueue(Queue *queue) {
    queue->front = 0;
    queue->rear = 0;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->notEmpty, NULL);
    pthread_cond_init(&queue->notFull, NULL);
}
/* Add an element to the queue. */
void enqueue(Queue *queue, int data) {
    pthread_mutex_lock(&queue->lock);
    while ((queue->rear + 1) % QUEUE_SIZE == queue->front) {
	pthread_cond_wait(&queue->notFull, &queue->lock);
    }
    queue->data[queue->rear] = data;
    queue->rear = (queue->rear + 1) % QUEUE_SIZE;
    pthread_cond_signal(&queue->notEmpty);
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

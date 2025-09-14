/**
 * @file producerConsumer.c
 * @brief This program demonstrates the producer-consumer problem using threads and mutexes.
 * @note This program's synchronization logic is flawed and can lead to deadlocks. It uses two mutexes
 * in a way that can cause a producer to wait for a consumer that is waiting for the producer.
 * A better approach would be to use condition variables.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define QUEUE_SIZE 10

/**
 * @struct Queue
 * @brief A circular queue.
 */
typedef struct {
    int data[QUEUE_SIZE];
    int front;
    int rear;
} Queue;

Queue sharedQueue;

void *producer(void *param);
void *consumer(void *param);

/**
 * @brief Adds an item to the queue.
 * @param sharedQueue A pointer to the queue.
 * @param data The item to add.
 */
void enQ(Queue *sharedQueue, int data) {
    sharedQueue->data[sharedQueue->rear] = data;
    sharedQueue->rear = (sharedQueue->rear + 1) % QUEUE_SIZE;
}

/**
 * @brief Removes an item from the queue.
 * @param sharedQueue A pointer to the queue.
 */
void deQ(Queue *sharedQueue) {
    sharedQueue->front = (sharedQueue->front + 1) % QUEUE_SIZE;
}

/**
 * @brief Returns the number of free spaces in the queue.
 * @param sharedQueue A pointer to the queue.
 * @return The number of free spaces.
 */
int noOfFreeSpaces(Queue *sharedQueue) {
    return (sharedQueue->front - sharedQueue->rear + QUEUE_SIZE) % QUEUE_SIZE;
}

/**
 * @brief Initializes the queue.
 * @param sharedQueue A pointer to the queue.
 */
void initQ(Queue *sharedQueue) {
    sharedQueue->front = 0;
    sharedQueue->rear = 0;
}

pthread_mutex_t p_mutex; /**< The producer mutex. */
pthread_mutex_t c_mutex; /**< The consumer mutex. */

/**
 * @brief The main function. It creates producer and consumer threads and waits for them to finish.
 * @return 0 on success, 1 on failure.
 */
int main() {
    int i;
    int nproducer;
    int mconsumer;
    printf("Enter the number of producer threads: ");
    scanf("%d", &nproducer);
    printf("Enter the number of consumer threads: ");
    scanf("%d", &mconsumer);

    pthread_t producer_tids[nproducer];
    pthread_t consumer_tids[mconsumer];
    int status;

    // Initialize the mutexes.
    pthread_mutex_init(&p_mutex, NULL);
    pthread_mutex_init(&c_mutex, NULL);

    // Lock the consumer mutex initially, since there is no data in the queue.
    pthread_mutex_lock(&c_mutex);

    // Initialize the shared queue.
    initQ(&sharedQueue);

    // Create the producer threads.
    for (i = 0; i < nproducer; i++) {
        status = pthread_create(&producer_tids[i], NULL, producer, NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_create() failed.\n");
            exit(1);
        }
    }

    // Create the consumer threads.
    for (i = 0; i < mconsumer; i++) {
        status = pthread_create(&consumer_tids[i], NULL, consumer, NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_create() failed.\n");
            exit(1);
        }
    }

    // Wait for the threads to finish.
    for (i = 0; i < nproducer; i++) {
        status = pthread_join(producer_tids[i], NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_join() failed.\n");
            exit(1);
        }
    }

    for (i = 0; i < mconsumer; i++) {
        status = pthread_join(consumer_tids[i], NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_join() failed.\n");
            exit(1);
        }
    }

    return 0;
}

/**
 * @brief The producer thread function. It produces random items and adds them to the queue.
 * @param param Not used.
 * @return NULL.
 */
void *producer(void *param) {
    int data;
    while (1) {
        pthread_mutex_lock(&p_mutex);

        if (noOfFreeSpaces(&sharedQueue) == 0) {
            fprintf(stderr, "Queue is full. Waiting for consumer to consume.\n");
            pthread_mutex_unlock(&p_mutex);
            pthread_mutex_lock(&c_mutex);
        }

        data = rand() % 100;
        enQ(&sharedQueue, data);

        printf("Enqueued Data: ");
        for (int i = sharedQueue.front; i != sharedQueue.rear; i = (i + 1) % QUEUE_SIZE) {
            printf("%d ", sharedQueue.data[i]);
        }
        printf("\n");

        fprintf(stderr, "Producer added data %d to the queue.\n", data);
        pthread_mutex_unlock(&p_mutex);
    }
}

/**
 * @brief The consumer thread function. It consumes items from the queue.
 * @param param Not used.
 * @return NULL.
 */
void *consumer(void *param) {
    int data;
    while (1) {
        pthread_mutex_lock(&c_mutex);

        if (sharedQueue.front == sharedQueue.rear) {
            fprintf(stderr, "Queue is empty. Waiting for producer to produce.\n");
            pthread_mutex_unlock(&c_mutex);
            pthread_mutex_lock(&p_mutex);
        }

        data = sharedQueue.data[sharedQueue.front];
        deQ(&sharedQueue);
        fprintf(stderr, "Consumer consumed data %d from the queue.\n", data);

        pthread_mutex_unlock(&c_mutex);
    }
}
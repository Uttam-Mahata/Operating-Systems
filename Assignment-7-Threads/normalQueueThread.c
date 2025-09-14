/**
 * @file normalQueueThread.c
 * @brief This program implements a thread-safe normal (non-circular) queue using POSIX threads.
 * It demonstrates the producer-consumer problem with a manager thread to dynamically add and remove producer and consumer threads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_QUEUE_SIZE 10
#define MAX_PRODUCER_THREADS 10
#define MAX_CONSUMER_THREADS 10
#define MAX_SLEEP_TIME 5

/**
 * @struct Queue
 * @brief A thread-safe normal queue.
 */
typedef struct {
    int items[MAX_QUEUE_SIZE];
    int size;  // Current size of queue
    pthread_mutex_t mutex;
    pthread_cond_t empty, full;
} Queue;

// Function prototypes
void initQueue(Queue *q);
void enQ(Queue *q, int item);
int deQ(Queue *q);
int isFull(Queue *q);
int isEmpty(Queue *q);
void clearResources();
void deleteProducer();
void deleteConsumer();

// Global variables
Queue queue;
pthread_t producerThreads[MAX_PRODUCER_THREADS];
pthread_t consumerThreads[MAX_CONSUMER_THREADS];
pthread_t managerThread;
int numProducers = 0, numConsumers = 0;

/**
 * @brief Initializes the queue.
 * @param q A pointer to the queue.
 */
void initQueue(Queue *q) {
    q->size = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->empty, NULL);
    pthread_cond_init(&q->full, NULL);
}

/**
 * @brief Adds an item to the end of the queue. This is the producer's operation.
 * @param q A pointer to the queue.
 * @param item The item to add.
 */
void enQ(Queue *q, int item) {
    pthread_mutex_lock(&q->mutex);
    
    while (isFull(q)) {
        pthread_cond_wait(&q->full, &q->mutex);
    }
    
    q->items[q->size] = item;
    q->size++;
    
    pthread_cond_signal(&q->empty);
    pthread_mutex_unlock(&q->mutex);
}

/**
 * @brief Removes an item from the front of the queue. This is the consumer's operation.
 * @param q A pointer to the queue.
 * @return The item removed from the queue.
 */
int deQ(Queue *q) {
    pthread_mutex_lock(&q->mutex);
    
    while (isEmpty(q)) {
        pthread_cond_wait(&q->empty, &q->mutex);
    }
    
    int item = q->items[0];
    
    // Shift all elements one position to the left.
    for (int i = 0; i < q->size - 1; i++) {
        q->items[i] = q->items[i + 1];
    }
    q->size--;
    
    pthread_cond_signal(&q->full);
    pthread_mutex_unlock(&q->mutex);
    return item;
}

/**
 * @brief Checks if the queue is full.
 * @param q A pointer to the queue.
 * @return 1 if the queue is full, 0 otherwise.
 */
int isFull(Queue *q) {
    return (q->size >= MAX_QUEUE_SIZE);
}

/**
 * @brief Checks if the queue is empty.
 * @param q A pointer to the queue.
 * @return 1 if the queue is empty, 0 otherwise.
 */
int isEmpty(Queue *q) {
    return (q->size == 0);
}

/**
 * @brief The producer thread function. It produces random items and adds them to the queue.
 * @param data A pointer to the producer's ID.
 * @return NULL.
 */
void *producer(void *data) {
    int producerId = *((int *)data);
    while (1) {
        srand(time(NULL) + producerId);
        int numItems = rand() % (MAX_QUEUE_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = rand() % 100;
            enQ(&queue, item);
            printf("Producer %d produced %d/%d item: %d\n",
                   producerId, i+1, numItems, item);
        }
        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
    return NULL;
}

/**
 * @brief The consumer thread function. It consumes items from the queue.
 * @param data A pointer to the consumer's ID.
 * @return NULL.
 */
void *consumer(void *data) {
    int consumerId = *((int *)data);
    while (1) {
        srand(time(NULL) + consumerId);
        int numItems = rand() % (MAX_QUEUE_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = deQ(&queue);
            printf("Consumer %d consumed %d/%d item: %d\n",
                   consumerId, i+1, numItems, item);
        }
        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
    return NULL;
}

/**
 * @brief Clears all resources, including threads, mutexes, and condition variables.
 */
void clearResources() {
    for (int i = 0; i < numProducers; ++i) {
        pthread_cancel(producerThreads[i]);
    }
    for (int i = 0; i < numConsumers; ++i) {
        pthread_cancel(consumerThreads[i]);
    }
    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.empty);
    pthread_cond_destroy(&queue.full);
    printf("All threads and resources cleared.\n");
    numProducers = 0;
    numConsumers = 0;
}

/**
 * @brief Deletes the last created producer thread.
 */
void deleteProducer() {
    if (numProducers > 0) {
        pthread_cancel(producerThreads[numProducers - 1]);
        printf("Producer thread %d deleted.\n", numProducers);
        numProducers--;
    } else {
        printf("No producer threads to delete.\n");
    }
}

/**
 * @brief Deletes the last created consumer thread.
 */
void deleteConsumer() {
    if (numConsumers > 0) {
        pthread_cancel(consumerThreads[numConsumers - 1]);
        printf("Consumer thread %d deleted.\n", numConsumers);
        numConsumers--;
    } else {
        printf("No consumer threads to delete.\n");
    }
}

/**
 * @brief The manager thread function. It provides a menu-driven interface to add, remove, and clear producer and consumer threads.
 * @param data Not used.
 * @return NULL.
 */
void *manager(void *data) {
    char choice;
    printf("Welcome to the manager thread!\n");
    do {
        printf("\nMenu:\n");
        printf("1. Add Producer\n");
        printf("2. Add Consumer\n");
        printf("3. Delete Producer\n");
        printf("4. Delete Consumer\n");
        printf("5. Clear All Threads, Resources and Exit.\n");
        printf("Enter your choice: ");
        scanf(" %c", &choice);

        switch (choice) {
            case '1':
                if (numProducers < MAX_PRODUCER_THREADS) {
                    pthread_create(&producerThreads[numProducers], NULL, 
                                 producer, &numProducers);
                    numProducers++;
                } else {
                    printf("Cannot add more producer threads.\n");
                }
                break;
            case '2':
                if (numConsumers < MAX_CONSUMER_THREADS) {
                    pthread_create(&consumerThreads[numConsumers], NULL, 
                                 consumer, &numConsumers);
                    numConsumers++;
                } else {
                    printf("Cannot add more consumer threads.\n");
                }
                break;
            case '3':
                deleteProducer();
                break;
            case '4':
                deleteConsumer();
                break;
            case '5':
                clearResources();
                printf("Exiting manager thread.\n");
                break;
            default:
                printf("Invalid choice!\n");
        }
    } while (choice != '5');

    return NULL;
}

/**
 * @brief The main function. It initializes the queue and creates the manager thread.
 * @return 0 on success.
 */
int main() {
    initQueue(&queue);
    pthread_create(&managerThread, NULL, manager, NULL);
    pthread_join(managerThread, NULL); // Wait for manager thread to finish
    return 0;
} 
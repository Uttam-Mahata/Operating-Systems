/**
 * @file circularQueueThread.c
 * @brief This program implements a thread-safe circular queue using POSIX threads.
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
 * @struct CircularQueue
 * @brief A thread-safe circular queue.
 */
typedef struct {
    int items[MAX_QUEUE_SIZE];
    int front, rear;
    pthread_mutex_t mutex;
    pthread_cond_t empty, full;
} CircularQueue;

// Function prototypes
void initQueue(CircularQueue *q);
void enQ(CircularQueue *q, int item);
int deQ(CircularQueue *q);
int isFull(CircularQueue *q);
int isEmpty(CircularQueue *q);
void clearResources();
void deleteProducer();
void deleteConsumer();

// Global variables
CircularQueue queue;
pthread_t producerThreads[MAX_PRODUCER_THREADS];
pthread_t consumerThreads[MAX_CONSUMER_THREADS];
pthread_t managerThread;
int numProducers = 0, numConsumers = 0;

/**
 * @brief Initializes the circular queue.
 * @param q A pointer to the circular queue.
 */
void initQueue(CircularQueue *q) {
    q->front = q->rear = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->empty, NULL);
    pthread_cond_init(&q->full, NULL);
}

/**
 * @brief Adds an item to the queue. This is the producer's operation.
 * @param q A pointer to the circular queue.
 * @param item The item to add.
 */
void enQ(CircularQueue *q, int item) {
    pthread_mutex_lock(&q->mutex);
    while (isFull(q)) {
        pthread_cond_wait(&q->full, &q->mutex);
    }
    q->items[q->rear] = item;
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    pthread_cond_signal(&q->empty);
    pthread_mutex_unlock(&q->mutex);
}

/**
 * @brief Removes an item from the queue. This is the consumer's operation.
 * @param q A pointer to the circular queue.
 * @return The item removed from the queue.
 */
int deQ(CircularQueue *q) {
    pthread_mutex_lock(&q->mutex);
    while (isEmpty(q)) {
        pthread_cond_wait(&q->empty, &q->mutex);
    }
    int item = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    pthread_cond_signal(&q->full);
    pthread_mutex_unlock(&q->mutex);
    return item;
}

/**
 * @brief Checks if the queue is full.
 * @param q A pointer to the circular queue.
 * @return 1 if the queue is full, 0 otherwise.
 */
int isFull(CircularQueue *q) {
    return ((q->rear + 1) % MAX_QUEUE_SIZE == q->front);
}

/**
 * @brief Checks if the queue is empty.
 * @param q A pointer to the circular queue.
 * @return 1 if the queue is empty, 0 otherwise.
 */
int isEmpty(CircularQueue *q) {
    return (q->front == q->rear);
}

/**
 * @brief The producer thread function. It produces random items and adds them to the queue.
 * @param data A pointer to the producer's ID.
 * @return NULL.
 */
void *producer(void *data) {
    int producerId = *((int *)data);
    while (1) {
        srand(time(NULL));
        int numItems = rand() % (MAX_QUEUE_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = rand() % 100;
            enQ(&queue, item);
            printf("Producer %d produced %d/%d item: %d\n", producerId, i + 1, numItems, item);
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
    srand(time(NULL));
    int consumerId = *((int *)data);
    while (1) {
        int numItems = rand() % (MAX_QUEUE_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = deQ(&queue);
            printf("Consumer %d consumed %d/%d item: %d\n", consumerId, i + 1, numItems, item);
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
                pthread_create(&producerThreads[numProducers], NULL, producer, &numProducers);
                numProducers++;
            } else {
                printf("Cannot add more producer threads.\n");
            }
            break;
        case '2':
            if (numConsumers < MAX_CONSUMER_THREADS) {
                pthread_create(&consumerThreads[numConsumers], NULL, consumer, &numConsumers);
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
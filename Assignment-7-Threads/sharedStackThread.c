/**
 * @file sharedStackThread.c
 * @brief This program implements a thread-safe shared stack using POSIX threads.
 * It demonstrates the producer-consumer problem with a manager thread to dynamically add and remove producer and consumer threads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_STACK_SIZE 10
#define MAX_PRODUCER_THREADS 10
#define MAX_CONSUMER_THREADS 10
#define MAX_SLEEP_TIME 5

/**
 * @struct SharedStack
 * @brief A thread-safe shared stack.
 */
typedef struct {
    int items[MAX_STACK_SIZE];
    int top;
    pthread_mutex_t mutex;
    pthread_cond_t empty, full;
} SharedStack;

// Function prototypes
void initStack(SharedStack *s);
void push(SharedStack *s, int item);
int pop(SharedStack *s);
int isFull(SharedStack *s);
int isEmpty(SharedStack *s);
void clearResources();
void deleteProducer();
void deleteConsumer();

// Global variables
SharedStack stack;
pthread_t producerThreads[MAX_PRODUCER_THREADS];
pthread_t consumerThreads[MAX_CONSUMER_THREADS];
pthread_t managerThread;
int numProducers = 0, numConsumers = 0;

/**
 * @brief Initializes the shared stack.
 * @param s A pointer to the shared stack.
 */
void initStack(SharedStack *s) {
    s->top = -1;
    pthread_mutex_init(&s->mutex, NULL);
    pthread_cond_init(&s->empty, NULL);
    pthread_cond_init(&s->full, NULL);
}

/**
 * @brief Pushes an item onto the stack. This is the producer's operation.
 * @param s A pointer to the shared stack.
 * @param item The item to push.
 */
void push(SharedStack *s, int item) {
    pthread_mutex_lock(&s->mutex);
    while (isFull(s)) {
        pthread_cond_wait(&s->full, &s->mutex);
    }
    s->items[++s->top] = item;
    pthread_cond_signal(&s->empty);
    pthread_mutex_unlock(&s->mutex);
}

/**
 * @brief Pops an item from the stack. This is the consumer's operation.
 * @param s A pointer to the shared stack.
 * @return The item popped from the stack.
 */
int pop(SharedStack *s) {
    pthread_mutex_lock(&s->mutex);
    while (isEmpty(s)) {
        pthread_cond_wait(&s->empty, &s->mutex);
    }
    int item = s->items[s->top--];
    pthread_cond_signal(&s->full);
    pthread_mutex_unlock(&s->mutex);
    return item;
}

/**
 * @brief Checks if the stack is full.
 * @param s A pointer to the shared stack.
 * @return 1 if the stack is full, 0 otherwise.
 */
int isFull(SharedStack *s) {
    return (s->top == MAX_STACK_SIZE - 1);
}

/**
 * @brief Checks if the stack is empty.
 * @param s A pointer to the shared stack.
 * @return 1 if the stack is empty, 0 otherwise.
 */
int isEmpty(SharedStack *s) {
    return (s->top == -1);
}

/**
 * @brief The producer thread function. It produces random items and pushes them onto the stack.
 * @param data A pointer to the producer's ID.
 * @return NULL.
 */
void *producer(void *data) {
    int producerId = *((int *)data);
    while (1) {
        srand(time(NULL) + producerId);
        int numItems = rand() % (MAX_STACK_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = rand() % 100;
            push(&stack, item);
            printf("Producer %d pushed %d/%d item: %d\n",
                   producerId, i+1, numItems, item);
        }
        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
    return NULL;
}

/**
 * @brief The consumer thread function. It consumes items from the stack.
 * @param data A pointer to the consumer's ID.
 * @return NULL.
 */
void *consumer(void *data) {
    int consumerId = *((int *)data);
    while (1) {
        srand(time(NULL) + consumerId);
        int numItems = rand() % (MAX_STACK_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = pop(&stack);
            printf("Consumer %d popped %d/%d item: %d\n",
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
    pthread_mutex_destroy(&stack.mutex);
    pthread_cond_destroy(&stack.empty);
    pthread_cond_destroy(&stack.full);
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
        printf("5. Clear All Threads, Resources and Exit\n");
        printf("Enter your choice: ");
        scanf(" %c", &choice);

        switch (choice) {
            case '1':
                if (numProducers < MAX_PRODUCER_THREADS) {
                    pthread_create(&producerThreads[numProducers], NULL, 
                                 producer, &numProducers);
                    printf("Added producer thread %d\n", numProducers + 1);
                    numProducers++;
                } else {
                    printf("Cannot add more producer threads.\n");
                }
                break;
            case '2':
                if (numConsumers < MAX_CONSUMER_THREADS) {
                    pthread_create(&consumerThreads[numConsumers], NULL, 
                                 consumer, &numConsumers);
                    printf("Added consumer thread %d\n", numConsumers + 1);
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
 * @brief The main function. It initializes the stack and creates the manager thread.
 * @return 0 on success.
 */
int main() {
    initStack(&stack);
    pthread_create(&managerThread, NULL, manager, NULL);
    pthread_join(managerThread, NULL); // Wait for manager thread to finish
    return 0;
} 
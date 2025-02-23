#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_STACK_SIZE 10
#define MAX_PRODUCER_THREADS 10
#define MAX_CONSUMER_THREADS 10
#define MAX_SLEEP_TIME 5

// Stack Structure
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

// Initialize the stack
void initStack(SharedStack *s) {
    s->top = -1;
    pthread_mutex_init(&s->mutex, NULL);
    pthread_cond_init(&s->empty, NULL);
    pthread_cond_init(&s->full, NULL);
}

// Push item to stack
void push(SharedStack *s, int item) {
    fprintf(stderr, "\t\t\t\tCalled push!\n");
    fprintf(stderr, "\t\t\t\tpush locking!\n");
    pthread_mutex_lock(&s->mutex);
    while (isFull(s)) {
        fprintf(stderr, "\t\t\t\tpush waiting!\n");
        pthread_cond_wait(&s->full, &s->mutex);
    }
    s->items[++s->top] = item;
    fprintf(stderr, "\t\t\t\tpush signalling!\n");
    pthread_cond_signal(&s->empty);
    fprintf(stderr, "\t\t\t\tpush unlocking!\n");
    pthread_mutex_unlock(&s->mutex);
}

// Pop item from stack
int pop(SharedStack *s) {
    fprintf(stderr, "\t\t\t\t\t\tCalled pop!\n");
    fprintf(stderr, "\t\t\t\t\t\tpop locking!\n");
    pthread_mutex_lock(&s->mutex);
    while (isEmpty(s)) {
        fprintf(stderr, "\t\t\t\t\t\tpop waiting!\n");
        pthread_cond_wait(&s->empty, &s->mutex);
    }
    int item = s->items[s->top--];
    fprintf(stderr, "\t\t\t\t\t\tpop signalling!\n");
    pthread_cond_signal(&s->full);
    fprintf(stderr, "\t\t\t\t\t\tpop unlocking!\n");
    pthread_mutex_unlock(&s->mutex);
    return item;
}

// Check if stack is full
int isFull(SharedStack *s) {
    return (s->top == MAX_STACK_SIZE - 1);
}

// Check if stack is empty
int isEmpty(SharedStack *s) {
    return (s->top == -1);
}

// Producer function
void *producer(void *data) {
    int producerId = *((int *)data);
    while (1) {
        srand(time(NULL) + producerId);
        int numItems = rand() % (MAX_STACK_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = rand() % 100;
            push(&stack, item);
            printf("\t\t\t\tProducer %d pushed %d/%d item: %d\n", 
                   producerId, i+1, numItems, item);
        }
        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
    return NULL;
}

// Consumer function
void *consumer(void *data) {
    int consumerId = *((int *)data);
    while (1) {
        srand(time(NULL) + consumerId);
        int numItems = rand() % (MAX_STACK_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = pop(&stack);
            printf("\t\t\t\t\t\tConsumer %d popped %d/%d item: %d\n", 
                   consumerId, i+1, numItems, item);
        }
        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
    return NULL;
}

// Clear resources function
void clearResources() {
    // Clear producer threads
    for (int i = 0; i < numProducers; ++i) {
        pthread_cancel(producerThreads[i]);
    }
    // Clear consumer threads
    for (int i = 0; i < numConsumers; ++i) {
        pthread_cancel(consumerThreads[i]);
    }
    // Destroy mutex and condition variables
    pthread_mutex_destroy(&stack.mutex);
    pthread_cond_destroy(&stack.empty);
    pthread_cond_destroy(&stack.full);
    printf("All threads and resources cleared.\n");
    // Reset thread counts
    numProducers = 0;
    numConsumers = 0;
}

// Delete producer function
void deleteProducer() {
    if (numProducers > 0) {
        pthread_cancel(producerThreads[numProducers - 1]);
        printf("Producer thread %d deleted.\n", numProducers);
        numProducers--;
    } else {
        printf("No producer threads to delete.\n");
    }
}

// Delete consumer function
void deleteConsumer() {
    if (numConsumers > 0) {
        pthread_cancel(consumerThreads[numConsumers - 1]);
        printf("Consumer thread %d deleted.\n", numConsumers);
        numConsumers--;
    } else {
        printf("No consumer threads to delete.\n");
    }
}

// Manager thread function
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

int main() {
    initStack(&stack);
    pthread_create(&managerThread, NULL, manager, NULL);
    pthread_join(managerThread, NULL); // Wait for manager thread to finish
    return 0;
} 
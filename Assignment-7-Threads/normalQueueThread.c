#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_QUEUE_SIZE 10
#define MAX_PRODUCER_THREADS 10
#define MAX_CONSUMER_THREADS 10
#define MAX_SLEEP_TIME 5

// Normal Queue Structure
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

// Initialize the queue
void initQueue(Queue *q) {
    q->size = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->empty, NULL);
    pthread_cond_init(&q->full, NULL);
}

// Add item to queue
void enQ(Queue *q, int item) {
    fprintf(stderr, "\t\t\t\tCalled enQ!\n");
    fprintf(stderr, "\t\t\t\tenQ locking!\n");
    pthread_mutex_lock(&q->mutex);
    
    while (isFull(q)) {
        fprintf(stderr, "\t\t\t\tenQ Waiting!\n");
        pthread_cond_wait(&q->full, &q->mutex);
    }
    
    // Shift elements to make room at the end
    q->items[q->size] = item;
    q->size++;
    
    fprintf(stderr, "\t\t\t\tenQ signalling!\n");
    pthread_cond_signal(&q->empty);
    fprintf(stderr, "\t\t\t\tenQ unlocking!\n");
    pthread_mutex_unlock(&q->mutex);
}

// Remove item from queue
int deQ(Queue *q) {
    fprintf(stderr, "\t\t\t\t\t\tCalled deQ!\n");
    fprintf(stderr, "\t\t\t\t\t\tdeQ locking!\n");
    pthread_mutex_lock(&q->mutex);
    
    while (isEmpty(q)) {
        fprintf(stderr, "\t\t\t\t\t\tdeQ waiting!\n");
        pthread_cond_wait(&q->empty, &q->mutex);
    }
    
    int item = q->items[0];
    
    // Shift all elements one position left
    for (int i = 0; i < q->size - 1; i++) {
        q->items[i] = q->items[i + 1];
    }
    q->size--;
    
    fprintf(stderr, "\t\t\t\t\t\tdeQ signalling!\n");
    pthread_cond_signal(&q->full);
    fprintf(stderr, "\t\t\t\t\t\tdeQ unlocking!\n");
    pthread_mutex_unlock(&q->mutex);
    return item;
}

// Check if queue is full
int isFull(Queue *q) {
    return (q->size >= MAX_QUEUE_SIZE);
}

// Check if queue is empty
int isEmpty(Queue *q) {
    return (q->size == 0);
}

// Producer function
void *producer(void *data) {
    int producerId = *((int *)data);
    while (1) {
        srand(time(NULL) + producerId);
        int numItems = rand() % (MAX_QUEUE_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = rand() % 100;
            enQ(&queue, item);
            printf("\t\t\t\tProducer %d produced %d/%d item: %d\n", 
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
        int numItems = rand() % (MAX_QUEUE_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = deQ(&queue);
            printf("\t\t\t\t\t\tConsumer %d consumed %d/%d item: %d\n", 
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
    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.empty);
    pthread_cond_destroy(&queue.full);
    printf("All threads and resources cleared.\n");
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

int main() {
    initQueue(&queue);
    pthread_create(&managerThread, NULL, manager, NULL);
    pthread_join(managerThread, NULL); // Wait for manager thread to finish
    return 0;
} 
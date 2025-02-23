#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_LIST_SIZE 10
#define MAX_PRODUCER_THREADS 10
#define MAX_CONSUMER_THREADS 10
#define MAX_SLEEP_TIME 5

// Node Structure
typedef struct Node {
    int data;
    struct Node* next;
} Node;

// Linked List Structure
typedef struct {
    Node* head;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t empty, full;
} LinkedList;

// Function prototypes
void initList(LinkedList *list);
void insertNode(LinkedList *list, int item);
int removeNode(LinkedList *list);
int isFull(LinkedList *list);
int isEmpty(LinkedList *list);
void clearResources();
void deleteProducer();
void deleteConsumer();

// Global variables
LinkedList list;
pthread_t producerThreads[MAX_PRODUCER_THREADS];
pthread_t consumerThreads[MAX_CONSUMER_THREADS];
pthread_t managerThread;
int numProducers = 0, numConsumers = 0;

// Initialize the list
void initList(LinkedList *list) {
    list->head = NULL;
    list->size = 0;
    pthread_mutex_init(&list->mutex, NULL);
    pthread_cond_init(&list->empty, NULL);
    pthread_cond_init(&list->full, NULL);
}

// Insert node at the end of list
void insertNode(LinkedList *list, int item) {
    fprintf(stderr, "\t\t\t\tCalled insertNode!\n");
    fprintf(stderr, "\t\t\t\tinsertNode locking!\n");
    pthread_mutex_lock(&list->mutex);
    
    while (isFull(list)) {
        fprintf(stderr, "\t\t\t\tinsertNode Waiting!\n");
        pthread_cond_wait(&list->full, &list->mutex);
    }

    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = item;
    newNode->next = NULL;

    if (list->head == NULL) {
        list->head = newNode;
    } else {
        Node* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
    list->size++;

    fprintf(stderr, "\t\t\t\tinsertNode signalling!\n");
    pthread_cond_signal(&list->empty);
    fprintf(stderr, "\t\t\t\tinsertNode unlocking!\n");
    pthread_mutex_unlock(&list->mutex);
}

// Remove node from front of list
int removeNode(LinkedList *list) {
    fprintf(stderr, "\t\t\t\t\t\tCalled removeNode!\n");
    fprintf(stderr, "\t\t\t\t\t\tremoveNode locking!\n");
    pthread_mutex_lock(&list->mutex);

    while (isEmpty(list)) {
        fprintf(stderr, "\t\t\t\t\t\tremoveNode waiting!\n");
        pthread_cond_wait(&list->empty, &list->mutex);
    }

    Node* temp = list->head;
    int item = temp->data;
    list->head = list->head->next;
    free(temp);
    list->size--;

    fprintf(stderr, "\t\t\t\t\t\tremoveNode signalling!\n");
    pthread_cond_signal(&list->full);
    fprintf(stderr, "\t\t\t\t\t\tremoveNode unlocking!\n");
    pthread_mutex_unlock(&list->mutex);
    return item;
}

// Check if list is full
int isFull(LinkedList *list) {
    return (list->size >= MAX_LIST_SIZE);
}

// Check if list is empty
int isEmpty(LinkedList *list) {
    return (list->size == 0);
}

// Producer function
void *producer(void *data) {
    int producerId = *((int *)data);
    while (1) {
        srand(time(NULL) + producerId);
        int numItems = rand() % (MAX_LIST_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = rand() % 100;
            insertNode(&list, item);
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
        int numItems = rand() % (MAX_LIST_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = removeNode(&list);
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
    // Free all nodes in the list
    Node* current = list.head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }
    // Destroy mutex and condition variables
    pthread_mutex_destroy(&list.mutex);
    pthread_cond_destroy(&list.empty);
    pthread_cond_destroy(&list.full);
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
    initList(&list);
    pthread_create(&managerThread, NULL, manager, NULL);
    pthread_join(managerThread, NULL); // Wait for manager thread to finish
    return 0;
} 
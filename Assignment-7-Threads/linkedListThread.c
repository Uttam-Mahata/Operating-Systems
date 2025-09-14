/**
 * @file linkedListThread.c
 * @brief This program implements a thread-safe linked list using POSIX threads.
 * It demonstrates the producer-consumer problem with a manager thread to dynamically add and remove producer and consumer threads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_LIST_SIZE 10
#define MAX_PRODUCER_THREADS 10
#define MAX_CONSUMER_THREADS 10
#define MAX_SLEEP_TIME 5

/**
 * @struct Node
 * @brief A node in the linked list.
 */
typedef struct Node {
    int data;
    struct Node* next;
} Node;

/**
 * @struct LinkedList
 * @brief A thread-safe linked list.
 */
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

/**
 * @brief Initializes the linked list.
 * @param list A pointer to the linked list.
 */
void initList(LinkedList *list) {
    list->head = NULL;
    list->size = 0;
    pthread_mutex_init(&list->mutex, NULL);
    pthread_cond_init(&list->empty, NULL);
    pthread_cond_init(&list->full, NULL);
}

/**
 * @brief Inserts a node at the end of the list. This is the producer's operation.
 * @param list A pointer to the linked list.
 * @param item The item to insert.
 */
void insertNode(LinkedList *list, int item) {
    pthread_mutex_lock(&list->mutex);
    
    while (isFull(list)) {
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

    pthread_cond_signal(&list->empty);
    pthread_mutex_unlock(&list->mutex);
}

/**
 * @brief Removes a node from the front of the list. This is the consumer's operation.
 * @param list A pointer to the linked list.
 * @return The item removed from the list.
 */
int removeNode(LinkedList *list) {
    pthread_mutex_lock(&list->mutex);

    while (isEmpty(list)) {
        pthread_cond_wait(&list->empty, &list->mutex);
    }

    Node* temp = list->head;
    int item = temp->data;
    list->head = list->head->next;
    free(temp);
    list->size--;

    pthread_cond_signal(&list->full);
    pthread_mutex_unlock(&list->mutex);
    return item;
}

/**
 * @brief Checks if the list is full.
 * @param list A pointer to the linked list.
 * @return 1 if the list is full, 0 otherwise.
 */
int isFull(LinkedList *list) {
    return (list->size >= MAX_LIST_SIZE);
}

/**
 * @brief Checks if the list is empty.
 * @param list A pointer to the linked list.
 * @return 1 if the list is empty, 0 otherwise.
 */
int isEmpty(LinkedList *list) {
    return (list->size == 0);
}

/**
 * @brief The producer thread function. It produces random items and adds them to the list.
 * @param data A pointer to the producer's ID.
 * @return NULL.
 */
void *producer(void *data) {
    int producerId = *((int *)data);
    while (1) {
        srand(time(NULL) + producerId);
        int numItems = rand() % (MAX_LIST_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = rand() % 100;
            insertNode(&list, item);
            printf("Producer %d produced %d/%d item: %d\n",
                   producerId, i+1, numItems, item);
        }
        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
    return NULL;
}

/**
 * @brief The consumer thread function. It consumes items from the list.
 * @param data A pointer to the consumer's ID.
 * @return NULL.
 */
void *consumer(void *data) {
    int consumerId = *((int *)data);
    while (1) {
        srand(time(NULL) + consumerId);
        int numItems = rand() % (MAX_LIST_SIZE - 1) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = removeNode(&list);
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
    Node* current = list.head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }
    pthread_mutex_destroy(&list.mutex);
    pthread_cond_destroy(&list.empty);
    pthread_cond_destroy(&list.full);
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
 * @brief The main function. It initializes the list and creates the manager thread.
 * @return 0 on success.
 */
int main() {
    initList(&list);
    pthread_create(&managerThread, NULL, manager, NULL);
    pthread_join(managerThread, NULL); // Wait for manager thread to finish
    return 0;
} 
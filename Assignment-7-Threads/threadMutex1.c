/**
 * @file threadMutex1.c
 * @brief This program demonstrates the use of mutex locks for thread synchronization.
 * It creates multiple producer and consumer threads that access a shared integer variable.
 * Mutual exclusion is ensured for the producer threads, but not for the consumer threads.
 * This means that some data may be consumed by more than one consumer thread, or some data may be overwritten
 * by a producer before it is consumed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define nproducer 2
#define nconsumer 3

/**
 * @struct passedData
 * @brief A structure to pass data to the threads.
 */
struct passedData {
    int *data;
    int producer_no;
    int consumer_no;
};

int sharedData = 0; /**< The shared integer variable. */

void *producer(void *param);
void *consumer(void *param);

pthread_mutex_t pc_mutex; /**< The mutex for producer-consumer synchronization. */

/**
 * @brief The main function. It creates producer and consumer threads and waits for them to complete.
 * @return 0 on success, 1 on failure.
 */
int main() {
    int i;
    pthread_t ptids[nproducer];
    pthread_t ctids[nconsumer];
    int status;
    pthread_attr_t attr;

    // Initialize the mutex.
    status = pthread_mutex_init(&pc_mutex, NULL);
    if (status != 0) {
        fprintf(stderr, "pthread_mutex_init() failed: %s.\n", strerror(status));
        exit(1);
    }

    // Set the default attributes for the threads.
    pthread_attr_init(&attr);

    // Initialize the shared data.
    sharedData = 0;

    // Create the producer threads.
    for (i = 0; i < nproducer; i++) {
        struct passedData *threadData = (struct passedData *)malloc(sizeof(struct passedData));
        threadData->data = &sharedData;
        threadData->producer_no = i;
        status = pthread_create(&ptids[i], &attr, producer, threadData);
        if (status != 0) {
            fprintf(stderr, "pthread_create() failed: %s.\n", strerror(status));
            exit(1);
        }
    }

    // Create the consumer threads.
    for (i = 0; i < nconsumer; i++) {
        struct passedData *threadData = (struct passedData *)malloc(sizeof(struct passedData));
        threadData->data = &sharedData;
        threadData->consumer_no = i;
        status = pthread_create(&ctids[i], &attr, consumer, threadData);
        if (status != 0) {
            fprintf(stderr, "pthread_create() failed: %s.\n", strerror(status));
            exit(1);
        }
    }

    // Wait for all threads to complete.
    for (i = 0; i < nproducer; i++) {
        status = pthread_join(ptids[i], NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_join() failed: %s.\n", strerror(status));
            exit(1);
        }
    }
    for (i = 0; i < nconsumer; i++) {
        status = pthread_join(ctids[i], NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_join() failed: %s.\n", strerror(status));
            exit(1);
        }
    }

    return 0;
}

/**
 * @brief The producer thread function. It increments the shared data variable.
 * @param param A pointer to a struct passedData containing the shared data and the producer number.
 * @return NULL.
 */
void *producer(void *param) {
    struct passedData *alldata = (struct passedData *)param;

    while (1) {
        fprintf(stderr, "I am producer thread [%d] got data = %d.\n", alldata->producer_no, *(alldata->data));

        // Lock the mutex before modifying the shared data.
        pthread_mutex_lock(&pc_mutex);
        (*(alldata->data))++;
        fprintf(stderr, "I am producer thread [%d] incremented data to %d.\n", alldata->producer_no, *(alldata->data));
        // Unlock the mutex after modifying the shared data.
        pthread_mutex_unlock(&pc_mutex);

        getchar(); // Wait for user input.
    }

    free(param);
    pthread_exit(0);
}

/**
 * @brief The consumer thread function. It reads the shared data variable.
 * @param param A pointer to a struct passedData containing the shared data and the consumer number.
 * @return NULL.
 */
void *consumer(void *param) {
    struct passedData *alldata = (struct passedData *)param;

    while (1) {
        // Note: The consumer does not lock the mutex, so it may read the shared data while a producer is modifying it.
        fprintf(stderr, "I am consumer thread [%d] got data = %d.\n", alldata->consumer_no, *(alldata->data));
        getchar(); // Wait for user input.
    }

    free(param);
    pthread_exit(0);
}
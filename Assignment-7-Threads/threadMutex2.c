/**
 * @file threadMutex2.c
 * @brief This program demonstrates a flawed attempt at producer-consumer synchronization using two mutexes.
 * A producer thread cannot produce any data unless the earlier data produced by some producer thread is consumed by some consumer thread.
 * Data produced by a producer thread can be consumed by only one consumer thread.
 * @note This program's synchronization logic is flawed and will lead to deadlocks.
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

pthread_mutex_t p_mutex; /**< The producer mutex. */
pthread_mutex_t c_mutex; /**< The consumer mutex. */

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

    // Initialize the mutexes.
    pthread_mutex_init(&p_mutex, NULL);
    pthread_mutex_init(&c_mutex, NULL);

    // Lock the consumer mutex initially, since there is no data.
    pthread_mutex_lock(&c_mutex);

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
        pthread_join(ptids[i], NULL);
    }
    for (i = 0; i < nconsumer; i++) {
        pthread_join(ctids[i], NULL);
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

        // Lock the producer mutex to prevent other producers from producing data.
        pthread_mutex_lock(&p_mutex);
        (*(alldata->data))++;
        fprintf(stderr, "I am producer thread [%d] incremented data to %d.\n", alldata->producer_no, *(alldata->data));
        // Unlock the consumer mutex to allow a consumer to read the data.
        pthread_mutex_unlock(&c_mutex);

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
        // Lock the consumer mutex to prevent other consumers from reading the data.
        pthread_mutex_lock(&c_mutex);
        fprintf(stderr, "I am consumer thread [%d] got data = %d.\n", alldata->consumer_no, *(alldata->data));
        // Unlock the producer mutex to allow a producer to produce more data.
        pthread_mutex_unlock(&p_mutex);

        getchar(); // Wait for user input.
    }

    free(param);
    pthread_exit(0);
}
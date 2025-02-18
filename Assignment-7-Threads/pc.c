#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* for thread functions */
#include <errno.h>   /* For the macros used here - EAGAIN, EINVAL, EPERM. etc.*/

#define QUEUE_SIZE 10

typedef struct
{
    int data[QUEUE_SIZE];
    int front;
    int rear;
} Queue;

Queue sharedQueue;

void *producer(void *param);

void *consumer(void *param);

void enQ(Queue *sharedQueue, int data)
{
    sharedQueue->data[sharedQueue->rear] = data;
    sharedQueue->rear = (sharedQueue->rear + 1) % QUEUE_SIZE;
}

void deQ(Queue *sharedQueue)
{
    sharedQueue->front = (sharedQueue->front + 1) % QUEUE_SIZE;
}

int noOfFreeSpaces(Queue *sharedQueue)
{
    return (sharedQueue->front - sharedQueue->rear + QUEUE_SIZE) % QUEUE_SIZE;
}

void initQ(Queue *sharedQueue)
{
    sharedQueue->front = 0;
    sharedQueue->rear = 0;
}

pthread_mutex_t p_mutex; /* producer mutex */

pthread_mutex_t c_mutex; /* consumer mutex */

int main()
{
    int i;
    int nproducer;
    int mconsumer;
    printf("Enter the number of producer threads: ");
    scanf("%d", &nproducer);
    printf("Enter the number of consumer threads: ");
    scanf("%d", &mconsumer);

    pthread_t producer_tids[nproducer];
    pthread_t consumer_tids[mconsumer];

    int status;         /* Used to store the return value (success/failure) of functions */

    pthread_attr_t attr; /*Set of thread attributes required to be passed in pthread_create() */

    /* Initialize the mutexes */
    // int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
    status = pthread_mutex_init(&p_mutex, NULL);
    status = pthread_mutex_init(&c_mutex, NULL);

    /* Block the consumers - no data is produced - producer()s however can proceed */
    /* int pthread_mutex_lock(pthread_mutex_t *mutex); */
    status = pthread_mutex_lock(&c_mutex); /* check status for error */

    if (status != 0)
    { /* pthread_mutex_init() failed */
        /* Do not use perror since pthreads functions  do  not  set  errno. */
        fprintf(stderr, "pthread_mutex_init() failed.\n");
        exit(1);
    }

    /* Initialize the sharedQueue */
    initQ(&sharedQueue);

    /* Create the producer threads */
    for (i = 0; i < 2; i++)
    {
        /* int pthread_create(pthread_t *restrict thread, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg); */
        status = pthread_create(&producer_tids[i], NULL, producer, NULL);
        if (status != 0)
        {
            fprintf(stderr, "pthread_create() failed.\n");
            exit(1);
        }
    }

    /* Create the consumer threads */
    for (i = 0; i < 3; i++)
    {
        /* int pthread_create(pthread_t *restrict thread, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg); */
        status = pthread_create(&consumer_tids[i], NULL, consumer, NULL);
        if (status != 0)
        {
            fprintf(stderr, "pthread_create() failed.\n");
            exit(1);
        }
    }

    /* Wait for the threads to finish */
    for (i = 0; i < nproducer; i++)
    {
        /* int pthread_join(pthread_t thread, void **retval); */
        status = pthread_join(producer_tids[i], NULL);
        if (status != 0)
        {
            fprintf(stderr, "pthread_join() failed.\n");
            exit(1);
        }
    }

    for (i = 0; i < mconsumer; i++)
    {
        /* int pthread_join(pthread_t thread, void **retval); */
        status = pthread_join(consumer_tids[i], NULL);
        if (status != 0)
        {
            fprintf(stderr, "pthread_join() failed.\n");
            exit(1);
        }
    }

    return 0;
}

void *producer(void *param)
{
    int status;
    int data;
    while (1)
    {
        /* int pthread_mutex_lock(pthread_mutex_t *mutex); */
        status = pthread_mutex_lock(&p_mutex);
        if (status != 0)
        {
            fprintf(stderr, "pthread_mutex_lock() failed.\n");
            exit(1);
        }

        if (noOfFreeSpaces(&sharedQueue) == 0)
        {
            fprintf(stderr, "Queue is full. Waiting for consumer to consume.\n");
            /* int pthread_mutex_unlock(pthread_mutex_t *mutex); */
            status = pthread_mutex_unlock(&p_mutex);
            if (status != 0)
            {
                fprintf(stderr, "pthread_mutex_unlock() failed.\n");
                exit(1);
            }
            /* int pthread_mutex_lock(pthread_mutex_t *mutex); */
            status = pthread_mutex_lock(&c_mutex);
            if (status != 0)
            {
                fprintf(stderr, "pthread_mutex_lock() failed.\n");
                exit(1);
            }
        }

        data = rand() % 100;
        enQ(&sharedQueue, data);
        fprintf(stderr, "Producer added data %d to the queue.\n", data);

        /* int pthread_mutex_unlock(pthread_mutex_t *mutex); */
        status = pthread_mutex_unlock(&p_mutex);
        if (status != 0)
        {
            fprintf(stderr, "pthread_mutex_unlock() failed.\n");
            exit(1);
        }
    }
}

void *consumer(void *param)
{
    int status;
    int data;
    while (1)
    {
        /* int pthread_mutex_lock(pthread_mutex_t *mutex); */
        status = pthread_mutex_lock(&c_mutex);
        if (status != 0)
        {
            fprintf(stderr, "pthread_mutex_lock() failed.\n");
            exit(1);
        }

        if (sharedQueue.front == sharedQueue.rear)
        {
            fprintf(stderr, "Queue is empty. Waiting for producer to produce.\n");
            /* int pthread_mutex_unlock(pthread_mutex_t *mutex); */
            status = pthread_mutex_unlock(&c_mutex);
            if (status != 0)
            {
                fprintf(stderr, "pthread_mutex_unlock() failed.\n");
                exit(1);
            }
            /* int pthread_mutex_lock(pthread_mutex_t *mutex); */
            status = pthread_mutex_lock(&p_mutex);
            if (status != 0)
            {
                fprintf(stderr, "pthread_mutex_lock() failed.\n");
                exit(1);
            }
        }

        data = sharedQueue.data[sharedQueue.front];
        deQ(&sharedQueue);
        fprintf(stderr, "Consumer consumed data %d from the queue.\n", data);

        /* int pthread_mutex_unlock(pthread_mutex_t *mutex); */
        status = pthread_mutex_unlock(&c_mutex);
        if (status != 0)
        {
            fprintf(stderr, "pthread_mutex_unlock() failed.\n");
            exit(1);
        }
    }
}
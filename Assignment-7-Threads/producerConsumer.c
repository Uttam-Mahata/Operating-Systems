
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* for thread functions */
#include <errno.h> /* For the macros used here - EAGAIN, EINVAL, EPERM. etc.*/

#define QUEUE_SIZE 10

typedef struct {
    int data[QUEUE_SIZE];
    int front;
    int rear;
} Queue;

Queue sharedQueue;

void *producer( void *param);
void *consumer( void *param);

void enQ(Queue *sharedQueue, int data) {
    sharedQueue->data[sharedQueue->rear] = data;
    sharedQueue->rear = (sharedQueue->rear + 1) % QUEUE_SIZE;
}

void deQ(Queue *sharedQueue) {
    sharedQueue->front = (sharedQueue->front + 1) % QUEUE_SIZE;
}
int noOfFreeSpaces(Queue *sharedQueue) {
    return (sharedQueue->front - sharedQueue->rear + QUEUE_SIZE) % QUEUE_SIZE;
}

void initQ(Queue *sharedQueue) {
    sharedQueue->front = 0;
    sharedQueue->rear = 0;
}




   pthread_mutex_t p_mutex; /* producer mutex */
   pthread_mutex_t c_mutex; /* consumer mutex */



int main() {
    int nproducer;
    int mconsumer;
    printf("Enter the number of producer threads: ");
    scanf("%d", &nproducer);
    printf("Enter the number of consumer threads: ");
    scanf("%d", &mconsumer);

    pthread_t producer_tids[nproducer];
    pthread_t consumer_tids[mconsumer];

    int status; /* Used to store the return value (success/failure) of functions */
   
    pthread_attr_t attr; /*Set of thread attributes required to be passed in pthread_create() */
    /* Initialize the mutexes */
       //int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
       status = pthread_mutex_init(&p_mutex, NULL);
       status = pthread_mutex_init(&c_mutex, NULL);
   
       /* Block the consumers - no data is produced - producer()s however can proceed */
       /* int pthread_mutex_lock(pthread_mutex_t *mutex); */
       status =  pthread_mutex_lock(&c_mutex); /* check status for error */
   
       if (status != 0) { /* pthread_mutex_init() failed */
           /* Do not use perror since pthreads functions  do  not  set  errno. */
           // perror("pthread_mutex_init() Failed: "); Cannot use this here
          
           /* Consult pthread_mutex_init() manual for information on return value for failure */
           fprintf(stderr, "pthread_mutex_init() failed: %s.\n", status == EAGAIN?"The system lacked the necessary resources (other than memory) to initialize another mutex.":status == ENOMEM?"Insufficient memory exists to initialize the mutex.":status == EPERM?"The caller does not have the privilege to perform the operation.":status==EINVAL?"The attributes object referenced by attr has the robust mutex attribute set without the process-shared attribute being set.":"Unknown Error");
           exit(1);
       }

         initQ(&sharedQueue);

         /* Set the default attributes to be used by pthread_create() */
            pthread_attr_init(&attr);  

    for(int i = 0; i < nproducer; i++) {
        status = pthread_create(&producer_tids[i], &attr, producer, NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
            exit(1);
        }
    }

 
    for(int i = 0; i < mconsumer; i++) {
        status = pthread_create(&consumer_tids[i], &attr, consumer, NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
            exit(1);
        }
    }


    for(int i = 0; i < nproducer; i++) {
        pthread_join(producer_tids[i], NULL);
    }

       

    for(int i = 0; i < mconsumer; i++) {
        pthread_join(consumer_tids[i], NULL);
    }

    return 0;
}


void *producer(void *param) {
    int data;
    while(1) {
        data = rand() % 100;
        pthread_mutex_lock(&p_mutex);
        enQ(&sharedQueue, data);
        printf("Produced: %d\n", data);
        pthread_mutex_unlock(&p_mutex);
    }
}

void *consumer(void *param) {
    int data;
    while(1) {
        pthread_mutex_lock(&c_mutex);
        data = sharedQueue.data[sharedQueue.front];
        deQ(&sharedQueue);
        printf("Consumed: %d\n", data);
        pthread_mutex_unlock(&c_mutex);
    }
}
/**
 * @file pthread1.c
 * @brief A simple program to demonstrate the creation and use of threads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/**
 * @brief The function that is executed by each thread. It prints a message to the console.
 * @param ptr A pointer to the message to be printed.
 * @return NULL.
 */
void *print_message_function(void *ptr);

/**
 * @brief The main function. It creates two threads, waits for them to complete, and then exits.
 * @return 0 on success.
 */
int main() {
    pthread_t thread1, thread2;
    char *message1 = "Thread 1";
    char *message2 = "Thread 2";
    int iret1, iret2;

    // Create two independent threads, each of which will execute the print_message_function.
    iret1 = pthread_create(&thread1, NULL, print_message_function, (void *)message1);
    iret2 = pthread_create(&thread2, NULL, print_message_function, (void *)message2);

    // Wait for the threads to complete before the main function continues.
    // Without this, the main function might exit before the threads have finished executing.
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Thread 1 returns: %d\n", iret1);
    printf("Thread 2 returns: %d\n", iret2);

    exit(0);
}

/**
 * @brief The function that is executed by each thread. It prints a message to the console.
 * @param ptr A pointer to the message to be printed.
 * @return NULL.
 */
void *print_message_function(void *ptr) {
    char *message;
    message = (char *)ptr;
    printf("%s \n", message);
    return NULL;
}

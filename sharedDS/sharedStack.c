/**
 * @file sharedStack.c
 * @brief This program implements a shared stack using semaphores and shared memory.
 * It demonstrates the basic operations of creating, pushing to, and popping from a shared stack.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>

#define STACK_SIZE 10

/**
 * @struct SharedStack
 * @brief A structure to hold the shared stack data.
 */
typedef struct {
    int data[STACK_SIZE];
    int top;
} SharedStack;

/**
 * @brief Performs a wait operation on a semaphore.
 * @param semid The semaphore ID.
 * @param sem_num The semaphore number in the set.
 */
void sem_wait(int semid, int sem_num) {
    struct sembuf sb = {sem_num, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Performs a signal operation on a semaphore.
 * @param semid The semaphore ID.
 * @param sem_num The semaphore number in the set.
 */
void sem_signal(int semid, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("sem_signal failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Creates a shared memory segment and a semaphore set.
 * @param shmid A pointer to the shared memory ID.
 * @param semid A pointer to the semaphore ID.
 */
void create_stack(int *shmid, int *semid) {
    // Create a shared memory segment for the stack.
    *shmid = shmget(IPC_PRIVATE, sizeof(SharedStack), IPC_CREAT | 0666);
    if (*shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    // Create a semaphore set with three semaphores:
    // 0: mutex (binary semaphore)
    // 1: empty (counting semaphore)
    // 2: full (counting semaphore)
    *semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    if (*semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the semaphores.
    semctl(*semid, 0, SETVAL, 1);             // mutex = 1
    semctl(*semid, 1, SETVAL, STACK_SIZE);    // empty = STACK_SIZE
    semctl(*semid, 2, SETVAL, 0);             // full = 0
}

/**
 * @brief Attaches to an existing shared memory segment.
 * @param shmid The shared memory ID.
 * @return A pointer to the shared stack.
 */
SharedStack *get_stack(int shmid) {
    SharedStack *stack = (SharedStack *)shmat(shmid, NULL, 0);
    if (stack == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    return stack;
}

/**
 * @brief Pushes a value onto the shared stack.
 * @param stack A pointer to the shared stack.
 * @param semid The semaphore ID.
 * @param value The value to push.
 */
void push(SharedStack *stack, int semid, int value) {
    sem_wait(semid, 1); // Wait for an empty slot.
    sem_wait(semid, 0); // Lock the mutex.

    stack->data[stack->top++] = value;

    sem_signal(semid, 0); // Unlock the mutex.
    sem_signal(semid, 2); // Signal that the stack is not empty.
}

/**
 * @brief Pops a value from the shared stack.
 * @param stack A pointer to the shared stack.
 * @param semid The semaphore ID.
 * @return The value popped from the stack.
 */
int pop(SharedStack *stack, int semid) {
    sem_wait(semid, 2); // Wait for a full slot.
    sem_wait(semid, 0); // Lock the mutex.

    int value = stack->data[--stack->top];

    sem_signal(semid, 0); // Unlock the mutex.
    sem_signal(semid, 1); // Signal that the stack is not full.

    return value;
}

/**
 * @brief The main function. It creates a shared stack, pushes and pops some values, and then cleans up the shared memory and semaphores.
 * @return 0 on success.
 */
int main() {
    int shmid, semid;
    SharedStack *stack;

    // Create the shared stack and semaphores.
    create_stack(&shmid, &semid);
    stack = get_stack(shmid);
    stack->top = 0;

    // Example usage.
    push(stack, semid, 10);
    push(stack, semid, 20);
    printf("Popped: %d\n", pop(stack, semid));
    printf("Popped: %d\n", pop(stack, semid));

    // Clean up.
    shmdt(stack);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, 0);

    return 0;
}

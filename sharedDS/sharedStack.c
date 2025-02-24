#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>

#define STACK_SIZE 10

// Define the shared stack structure
typedef struct {
    int data[STACK_SIZE];
    int top;
} SharedStack;

// Semaphore operations
void sem_wait(int semid, int sem_num) {
    struct sembuf sb = {sem_num, -1, 0};
    if (semop(semid, &sb, 1) {
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }
}

void sem_signal(int semid, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    if (semop(semid, &sb, 1) {
        perror("sem_signal failed");
        exit(EXIT_FAILURE);
    }
}

// Create shared memory and semaphores
void create_stack(int *shmid, int *semid) {
    // Create shared memory
    *shmid = shmget(IPC_PRIVATE, sizeof(SharedStack), IPC_CREAT | 0666);
    if (*shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    // Create semaphores
    *semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    if (*semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores
    semctl(*semid, 0, SETVAL, 1); // mutex = 1
    semctl(*semid, 1, SETVAL, STACK_SIZE); // empty = STACK_SIZE
    semctl(*semid, 2, SETVAL, 0); // full = 0
}

// Attach to shared memory and semaphores
SharedStack *get_stack(int shmid, int semid) {
    SharedStack *stack = (SharedStack *)shmat(shmid, NULL, 0);
    if (stack == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    return stack;
}

// Push operation
void push(SharedStack *stack, int semid, int value) {
    sem_wait(semid, 1); // Wait for empty slot
    sem_wait(semid, 0); // Lock mutex

    stack->data[stack->top++] = value;

    sem_signal(semid, 0); // Unlock mutex
    sem_signal(semid, 2); // Signal full
}

// Pop operation
int pop(SharedStack *stack, int semid) {
    sem_wait(semid, 2); // Wait for full slot
    sem_wait(semid, 0); // Lock mutex

    int value = stack->data[--stack->top];

    sem_signal(semid, 0); // Unlock mutex
    sem_signal(semid, 1); // Signal empty

    return value;
}

int main() {
    int shmid, semid;
    SharedStack *stack;

    // Create the stack
    create_stack(&shmid, &semid);
    stack = get_stack(shmid, semid);
    stack->top = 0; // Initialize top

    // Example usage
    push(stack, semid, 10);
    push(stack, semid, 20);
    printf("Popped: %d\n", pop(stack, semid));
    printf("Popped: %d\n", pop(stack, semid));

    // Cleanup
    shmdt(stack);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}

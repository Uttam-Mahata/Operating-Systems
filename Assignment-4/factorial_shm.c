#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#define READY 1
#define BUSY 0

// Structure to hold the shared data and flags
typedef struct {
    int number;
    long long factorial;
    int parent_flag;
    int child_flag;
} shared_data;

int shmid;

// Signal handler for releasing shared memory
void releaseSHM(int signum) {
    int status = shmctl(shmid, IPC_RMID, NULL);
    if (status == 0) {
        fprintf(stderr, "Removed shared memory with id = %d.\n", shmid);
    } else if (status == -1) {
        perror("Cannot remove shared memory");
    } else {
        fprintf(stderr, "shmctl() returned wrong value.\n");
    }

    kill(0, SIGKILL);
}

long long factorial(int n) {
    if (n < 0) return -1;
    if (n > 20) return -2; // To show overflow
    if (n == 0) return 1;
    return n * factorial(n - 1);
}

int main() {
    pid_t pid;
    int status;
    shared_data *shared;
    signal(SIGINT, releaseSHM);

    // Create shared memory
    shmid = shmget(IPC_PRIVATE, sizeof(shared_data), IPC_CREAT | 0777);
    if (shmid == -1) {
        perror("shmget() failed");
        exit(1);
    }
    printf("Shared memory ID: %d\n", shmid);

    // Attach shared memory
    shared = (shared_data *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat() failed");
        exit(1);
    }

    // Initialize shared data and flags
    shared->parent_flag = READY;
    shared->child_flag = BUSY;
    shared->number = 0;
    shared->factorial = 0;
    srand(time(NULL)); // random number generator

    pid = fork();

    if (pid == 0) { // Child process
        printf("Child process started.\n");
        while (1) {
            // if parent has put a new number
            if (shared->parent_flag == BUSY) {
                int num = shared->number;
                printf("Child: Received number %d from parent.\n", num);

                // Calculate factorial
                long long fact = factorial(num);
                if (fact == -1) { // Error - negative input
                    fprintf(stderr, "Child: Factorial of %d is undefined (negative).\n", num);
                    shared->factorial = -1; 
                } else if (fact == -2) {
                    fprintf(stderr, "Child: Factorial of %d caused overflow.\n", num);
                    shared->factorial = -2; 
                } else {
                    shared->factorial = fact;
                    printf("Child: Factorial of %d is %lld.\n", num, fact);
                }

                // Update shared memory and flags
                shared->child_flag = READY;
                shared->parent_flag = READY;
                sleep(1);
            } else {
                // Wait for parent to put a new number
                sleep(1);
            }
        }
        exit(0);
    } else { // Parent process
        printf("Parent process started.\n");
        int i = 0;
        while (i < 10) {
            // Generate a random number
            int num = rand() % 22;
            printf("Parent: Generated number %d.\n", num);

            // Update shared memory and flags
            shared->number = num;
            shared->parent_flag = BUSY;
            shared->child_flag = BUSY;

            // Wait for child to calculate factorial
            while (shared->child_flag == BUSY) {
                sleep(1);
            }

            if (shared->factorial == -1) {
                printf("Parent: Received -1  negative input from child.\n");
            } else if (shared->factorial == -2) {
                printf("Parent: Received -2  overflow from child.\n");
            } else {
                printf("Parent: Received factorial %lld from child.\n", shared->factorial);
            }

            sleep(2);
            i++;
        }

        // Clean up
        kill(pid, SIGINT); // Signal child to terminate
        wait(&status);
        printf("Child exited with status %d\n", status);

        status = shmctl(shmid, IPC_RMID, NULL);
        if (status == 0) {
            fprintf(stderr, "Remove shared memory with id = %d.\n", shmid);
        } else if (status == -1) {
            perror("Cannot remove shared memory");
        } else {
            fprintf(stderr, "shmctl() returned wrong value.\n");
        }
        exit(0);
    }

    return 0;
}
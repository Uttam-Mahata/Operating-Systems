/**
 * @file factorial_shm.c
 * @brief This program demonstrates the use of shared memory for inter-process communication.
 * The parent process generates random numbers and passes them to the child process through shared memory.
 * The child process calculates the factorial of the numbers and passes the result back to the parent, also through shared memory.
 */

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

/**
 * @brief A structure to hold the data that will be shared between the parent and child processes.
 */
typedef struct {
    int number;             /**< The number to calculate the factorial of. */
    long long factorial;    /**< The result of the factorial calculation. */
    int parent_flag;        /**< A flag to indicate if the parent is ready. */
    int child_flag;         /**< A flag to indicate if the child is ready. */
} shared_data;

int shmid; /**< The ID of the shared memory segment. */

/**
 * @brief A signal handler to release the shared memory segment when the program is interrupted.
 * @param signum The signal number.
 */
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

/**
 * @brief Calculates the factorial of a number.
 * @param n The number to calculate the factorial of.
 * @return The factorial of the number, or -1 if the number is negative, or -2 if the factorial causes an overflow.
 */
long long factorial(int n) {
    if (n < 0) return -1;
    if (n > 20) return -2; // To show overflow
    if (n == 0) return 1;
    return n * factorial(n - 1);
}

/**
 * @brief The main function. It creates a shared memory segment, forks a child process, and then the parent and child
 * processes communicate through the shared memory to calculate the factorials of random numbers.
 * @return 0 on success, 1 on failure.
 */
int main() {
    pid_t pid;
    int status;
    shared_data *shared;
    signal(SIGINT, releaseSHM);

    // Create a shared memory segment.
    shmid = shmget(IPC_PRIVATE, sizeof(shared_data), IPC_CREAT | 0777);
    if (shmid == -1) {
        perror("shmget() failed");
        exit(1);
    }
    printf("Shared memory ID: %d\n", shmid);

    // Attach the shared memory segment to the address space of the process.
    shared = (shared_data *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat() failed");
        exit(1);
    }

    // Initialize the shared data and flags.
    shared->parent_flag = READY;
    shared->child_flag = BUSY;
    shared->number = 0;
    shared->factorial = 0;
    srand(time(NULL)); // Seed the random number generator.

    pid = fork();

    if(pid == -1) {
        perror("fork() failed");
        exit(1);
    }

    if (pid == 0) { // Child process
        printf("Child process started.\n");
        while (1) {
            // If the parent has put a new number in the shared memory...
            if (shared->parent_flag == BUSY) {
                int num = shared->number;
                printf("Child: Received number %d from parent.\n", num);

                // Calculate the factorial.
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

                // Update the shared memory and flags to signal to the parent that the calculation is complete.
                shared->child_flag = READY;
                shared->parent_flag = READY;
                sleep(1);
            } else {
                // Wait for the parent to put a new number in the shared memory.
                sleep(1);
            }
        }
        exit(0);
    } else { // Parent process
        printf("Parent process started.\n");
        int i = 0;
        while (i < 10) {
            // Generate a random number.
            int num = rand() % 22;
            printf("Parent: Generated number %d.\n", num);

            // Update the shared memory and flags to signal to the child that a new number is available.
            shared->number = num;
            shared->parent_flag = BUSY;
            shared->child_flag = BUSY;

            // Wait for the child to calculate the factorial.
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

        // Clean up the shared memory and terminate the child process.
        kill(pid, SIGINT); // Signal the child to terminate.
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
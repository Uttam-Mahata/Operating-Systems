/**
 * @file test.c
 * @brief This program demonstrates the use of shared memory to pass an array of integers from a parent process to multiple child processes.
 * @note This program has some logical errors. Each child process will have access to the shared memory, but they will be racing to write to it.
 * The parent process also does not wait for the child processes to finish, and it does not clean up the shared memory segment.
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

/**
 * @brief The main function. It reads an array of 10 integers from the user, creates a shared memory segment, and then creates 10 child processes.
 * Each child process is intended to write one of the integers to the shared memory.
 * @return 0 on success.
 */
int main() {
    // Taking 10 integers as input.
    int n = 10;
    int arr[n];

    printf("Enter 10 integers:\n");
    for (int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }

    // Creating a shared memory segment to be used by the child processes.
    pid_t pid;
    int shmid;
    shmid = shmget(IPC_PRIVATE, sizeof(int) * n, IPC_CREAT | 0777);

    // Create a child process for each integer in the array.
    for (int i = 0; i < n; i++) {
        pid = fork();

        if (pid == 0) {
            // This is the child process.
            // Attach the shared memory segment to the address space of the process.
            int *shared = (int *)shmat(shmid, NULL, 0);
            // Write the integer to the shared memory.
            shared[i] = arr[i];
            printf("Child %d: %d\n", i, arr[i]);

            // Detach the shared memory segment.
            shmdt(shared);
            exit(0);
        }
    }

    // The parent process should wait for all children to finish and then clean up the shared memory.
    // This part is missing from the original code.
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
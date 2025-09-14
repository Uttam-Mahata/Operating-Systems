/**
 * @file worker.c
 * @brief This program is the worker in a client-server application that uses shared memory for inter-process communication.
 * The worker waits for the server to put a string in a shared memory segment, processes the string (in this case, by converting it to uppercase),
 * and then puts the result back in the shared memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctype.h>

/**
 * @brief A structure to hold the data that will be shared between the server and worker processes.
 */
struct task {
    char data[100];     /**< The string to be processed. */
    pid_t worker_pid;   /**< The PID of the worker process. */
    int status;         /**< The status of the task. 0: idle, 1: task available, 2: task taken, 3: task complete, 4: result consumed, -1: terminate */
};

/**
 * @brief The main function. It attaches to the shared memory segment created by the server, and then enters a loop
 * where it waits for tasks from the server, processes them, and puts the results back in the shared memory.
 * @return 0 on success, 1 on failure.
 */
int main() {
    key_t shmkey;
    int shmid;
    struct task *solve;

    // ftok() generates a key for the shared memory segment.
    // It must use the same file path and project ID as the server to get the same key.
    shmkey = ftok("/tmp", 'S');
    if (shmkey == -1) {
        perror("Worker: ftok failed");
        exit(1);
    }

    // shmget() gets the ID of the existing shared memory segment created by the server.
    shmid = shmget(shmkey, sizeof(struct task), 0666);
    if (shmid == -1) {
        perror("Worker: shmget failed");
        exit(1);
    }

    printf("Worker: Shared memory ID: %d\n", shmid);

    // shmat() attaches the shared memory segment to the address space of the process.
    solve = (struct task *)shmat(shmid, NULL, 0);
    if (solve == (struct task *)-1) {
        perror("Worker: shmat failed");
        exit(1);
    }

    printf("Worker: Attached to shared memory.\n");

    while (solve->status != -1) {
        // Wait for the server to put data in the shared memory.
        while (solve->status != 1 && solve->status != -1) {
            sleep(1);
        }

        if (solve->status == -1) break; // Exit if the server is terminating.

        // Process the string. In this case, convert it to uppercase.
        solve->status = 2;
        solve->worker_pid = getpid();
        printf("Worker PID %d: Processing string: %s\n", solve->worker_pid, solve->data);

        for (int i = 0; i < strlen(solve->data); i++) {
            solve->data[i] = toupper(solve->data[i]);
        }

        // Set the status to 3 to indicate that the task is complete.
        solve->status = 3;
        printf("Worker PID %d: String processing done.\n", solve->worker_pid);
    }

    printf("Worker PID %d: Exiting.\n", getpid());
    shmdt(solve); // Detach the shared memory segment.
    return 0;
}
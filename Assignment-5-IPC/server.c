/**
 * @file server.c
 * @brief This program is the server in a client-server application that uses shared memory for inter-process communication.
 * The server generates random strings and puts them in a shared memory segment. It then waits for a worker process to
 * process the string and put the result back in the shared memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>

/**
 * @brief A structure to hold the data that will be shared between the server and worker processes.
 */
struct task {
    char data[100];     /**< The string to be processed. */
    pid_t worker_pid;   /**< The PID of the worker process. */
    int status;         /**< The status of the task. 0: idle, 1: task available, 2: task taken, 3: task complete, 4: result consumed, -1: terminate */
};

int shmid;              /**< The ID of the shared memory segment. */
struct task *solve;     /**< A pointer to the shared memory segment. */

/**
 * @brief A signal handler for the SIGINT signal (Ctrl+C).
 * This function is responsible for cleaning up the shared memory segment before the program terminates.
 * @param sig The signal number.
 */
void cleanup(int sig) {
    printf("Received signal is %d .Cleaning up and exiting on Ctrl+C...\n", sig);
    // Set the status to -1 to signal the worker to terminate.
    solve->status = -1;
    // Detach the shared memory segment.
    shmdt(solve);
    // Remove the shared memory segment.
    shmctl(shmid, IPC_RMID, NULL);
    exit(0);
}

/**
 * @brief The main function. It creates a shared memory segment, and then enters a loop where it generates random strings,
 * puts them in the shared memory, and waits for a worker process to process them.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char* argv[]) {
    key_t shmkey;

    signal(SIGINT, cleanup);

    // Seed the random number generator.
    srand(time(NULL));

    // ftok() generates a key for the shared memory segment.
    // It uses a file path and a project ID to generate a unique key.
    // Using /tmp is a common practice to make the key generation more portable.
    shmkey = ftok("/tmp", 'S');
    if(shmkey == -1) {
        perror("ftok failed");
        exit(1);
    }

    // shmget() creates a new shared memory segment.
    shmid = shmget(shmkey, sizeof(struct task), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Server: shmget failed");
        exit(1);
    }

    // shmat() attaches the shared memory segment to the address space of the process.
    solve = (struct task *)shmat(shmid, NULL, 0);
    if (solve == (struct task *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize the shared memory.
    solve->status = 0;
    printf("Shared memory initialized.\n");

    while (1) {
        // Wait until the worker is ready for a new task.
        while(solve->status != 0 && solve->status != 4) {
            sleep(1);
            if(solve->status == -1) break;
        }
        if(solve->status == -1) break;

        // Generate a random string.
        int len = (rand() % 20) + 1;
        for (int i = 0; i < len; i++) {
            solve->data[i] = 'a' + (rand() % 26);
        }
        solve->data[len] = '\0';

        printf("Generated string: %s\n", solve->data);
        // Set the status to 1 to indicate that a new task is available.
        solve->status = 1;

        // Wait for the worker to process the string.
        while (solve->status != 3 && solve->status != -1) {
            sleep(1);
        }

        if (solve->status == -1) break; // Exit if the server is terminating.

        // The worker has processed the string and put the result in solve->data.
        // In this example, the worker converts the string to uppercase.
        printf("Worker PID %d processed string: %s\n", solve->worker_pid, solve->data);
        // Set the status to 4 to indicate that the result has been consumed.
        solve->status = 4;

        sleep(2); // Wait before generating the next string.
    }

    printf("Server: Exiting main loop.\n");
    cleanup(0);

    return 0;
}
/**
 * @file sample_program_shm.c
 * @brief This program demonstrates a simple use of shared memory for inter-process communication.
 * The parent process writes integers to a shared memory segment, and the child process reads them.
 */

#include <stdio.h>
#include <unistd.h> /* for fork() */
#include <sys/wait.h> /* for wait() */
#include <sys/types.h> /* for wait() kill(2)*/
#include <sys/ipc.h> /* for shmget() shmctl() */
#include <sys/shm.h> /* for shmget() shmctl() */
#include <signal.h> /* for signal(2) kill(2) */
#include <errno.h> /* for perror */
#include <stdlib.h>

/**
 * @brief The ID of the shared memory segment.
 * This is a global variable so that the signal handler can access it.
 */
int shmid;

/**
 * @brief A signal handler for the SIGINT signal (Ctrl+C).
 * This function is responsible for releasing the shared memory segment before the program terminates.
 * @param signum The signal number.
 */
void releaseSHM(int signum) {
    int status;
    // shmctl() is used to perform control operations on a shared memory segment.
    // IPC_RMID marks the segment to be destroyed.
    status = shmctl(shmid, IPC_RMID, NULL);
    if (status == 0) {
        fprintf(stderr, "Remove shared memory with id = %d.\n", shmid);
    } else if (status == -1) {
        fprintf(stderr, "Cannot remove shared memory with id = %d.\n", shmid);
    } else {
        fprintf(stderr, "shmctl() returned wrong value while removing shared memory with id = %d.\n", shmid);
    }

    // kill(0, SIGKILL) sends the SIGKILL signal to all processes in the process group.
    // This is a forceful way to terminate all processes.
    status = kill(0, SIGKILL);
    if (status == 0) {
        fprintf(stderr, "kill successful.\n"); // This line may not be executed because the process is killed before it can print.
    } else if (status == -1) {
        perror("kill failed.\n");
        fprintf(stderr, "Cannot remove shared memory with id = %d.\n", shmid);
    } else {
        fprintf(stderr, "kill(2) returned wrong value.\n");
    }
}

/**
 * @brief The main function. It creates a shared memory segment, forks a child process, and then the parent writes to the shared memory
 * while the child reads from it.
 * @return 0 on success, 1 on failure.
 */
int main() {
    int status;
    pid_t pid = 0;
    pid_t p1 = 0;

    // Install the signal handler for SIGINT.
    signal(SIGINT, releaseSHM);

    // shmget() creates a new shared memory segment or gets the ID of an existing one.
    // IPC_PRIVATE creates a new segment that can only be accessed by the creating process and its descendants.
    // IPC_CREAT creates the segment if it doesn't already exist.
    // 0777 sets the permissions for the segment.
    shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777);

    if (shmid == -1) {
        perror("shmget() failed: ");
        exit(1);
    }

    printf("shmget() returns shmid = %d.\n", shmid);

    p1 = fork();
    if (p1 == 0) { // This is the child process.
        int i;
        int *pi_child;

        // shmat() attaches the shared memory segment to the address space of the process.
        pi_child = shmat(shmid, NULL, 0);
        if (pi_child == (void *)-1) {
            perror("shmat() failed at child: ");
            exit(1);
        }

        for (i = 0; i < 50; i++) {
            printf("Child Reads %d.\n", *pi_child);
            getchar();
        }
        exit(0);

    } else { // This is the parent process.
        int i;
        int *pi_parent;

        // shmat() attaches the shared memory segment to the address space of the process.
        pi_parent = shmat(shmid, NULL, 0);
        if (pi_parent == (void *)-1) {
            perror("shmat() failed at parent: ");
            exit(1);
        }

        for (i = 0; i < 50; i++) {
            *pi_parent = i;
            printf("Parent writes %d.\n", *pi_parent);
            getchar();
        }
        pid = wait(&status);
        printf("pid = %d status = %d!\n", pid, status);
        exit(0);
    }
}
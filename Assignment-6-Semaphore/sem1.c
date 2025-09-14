/**
 * @file sem1.c
 * @brief This program demonstrates the use of a binary semaphore for mutual exclusion between a parent and child process.
 */

#include <stdio.h>
#include <sys/types.h> /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/ipc.h>   /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/sem.h>   /* for semget(2) semop(2) semctl(2) */
#include <unistd.h>    /* for fork(2) */
#include <stdlib.h>    /* for exit(3) */
#include <sys/wait.h>  /* for wait() */

#define NO_SEM 1

/**
 * @brief P (wait) operation on a semaphore.
 */
#define P(s) semop(s, &Pop, 1)

/**
 * @brief V (signal) operation on a semaphore.
 */
#define V(s) semop(s, &Vop, 1)

struct sembuf Pop;
struct sembuf Vop;

/**
 * @union semun
 * @brief A union for semaphore control operations.
 */
union semun {
    int val;                /**< Value for SETVAL */
    struct semid_ds *buf;   /**< Buffer for IPC_STAT, IPC_SET */
    unsigned short *array;  /**< Array for GETALL, SETALL */
    struct seminfo *__buf;  /**< Buffer for IPC_INFO (Linux-specific) */
};

/**
 * @brief The main function. It creates a binary semaphore, forks a child process, and then both the parent and child
 * processes use the semaphore to ensure mutual exclusion when printing to the console.
 * @return 0 on success, 1 on failure.
 */
int main() {
    key_t mykey;
    pid_t pid;
    int semid;
    int status;
    union semun setvalArg;

    setvalArg.val = 1;

    // Initialize the P (wait) operation.
    Pop.sem_num = 0;
    Pop.sem_op = -1;
    Pop.sem_flg = SEM_UNDO;

    // Initialize the V (signal) operation.
    Vop.sem_num = 0;
    Vop.sem_op = 1;
    Vop.sem_flg = SEM_UNDO;

    // Generate a key for the semaphore set.
    mykey = ftok("/tmp", 'S');
    if (mykey == -1) {
        perror("ftok() failed");
        exit(1);
    }

    // Create a semaphore set with one semaphore.
    semid = semget(mykey, NO_SEM, IPC_CREAT | 0777);
    if (semid == -1) {
        perror("semget() failed");
        exit(1);
    }

    // Initialize the semaphore to 1 (making it a binary semaphore).
    status = semctl(semid, 0, SETVAL, setvalArg);
    if (status == -1) {
        perror("semctl() failed");
        exit(1);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork() failed");
        // Clean up the semaphore before exiting.
        semctl(semid, 0, IPC_RMID);
        exit(1);
    }

    if (pid == 0) {
        // This is the child process.
        P(semid); // Wait for the semaphore.
        fprintf(stdout, "Child.\n");
        V(semid); // Signal the semaphore.
        exit(0);
    } else {
        // This is the parent process.
        P(semid); // Wait for the semaphore.
        fprintf(stdout, "Parent.\n");
        V(semid); // Signal the semaphore.
        // Wait for the child to finish.
        wait(NULL);
        // Clean up the semaphore.
        semctl(semid, 0, IPC_RMID);
    }

    return 0;
}
/**
 * @file matrix_multiplication_shm.c
 * @brief This program demonstrates parallel matrix multiplication using shared memory and multiple child processes.
 * The parent process reads two matrices from the user and stores them in shared memory. It then creates a child process
 * for each row of the output matrix. Each child process calculates one row of the result matrix and stores it in shared memory.
 * The parent process then waits for all child processes to complete and prints the result matrix.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_N 10
#define MAX_M 10
#define MAX_P 10

/**
 * @brief A structure to hold the matrices and their dimensions that will be shared between the parent and child processes.
 */
typedef struct {
    int a[MAX_N][MAX_M];    /**< The first matrix. */
    int b[MAX_M][MAX_P];    /**< The second matrix. */
    int c[MAX_N][MAX_P];    /**< The result matrix. */
    int n, m, p;            /**< The dimensions of the matrices. */
} SharedData;

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
    }

    kill(0, SIGKILL);  // Kill all processes in the process group.
}

/**
 * @brief Calculates one row of the result matrix.
 * This function is executed by each child process.
 * @param shared A pointer to the shared data structure.
 * @param row_num The row number to calculate.
 */
void calculate_row(SharedData *shared, int row_num) {
    for (int j = 0; j < shared->p; j++) {
        shared->c[row_num][j] = 0;
        for (int k = 0; k < shared->m; k++) {
            shared->c[row_num][j] += shared->a[row_num][k] * shared->b[k][j];
        }
    }
}

/**
 * @brief The main function. It creates a shared memory segment, reads two matrices from the user, creates a child process
 * for each row of the result matrix, waits for the children to complete, and then prints the result matrix.
 * @return 0 on success, 1 on failure.
 */
int main() {
    SharedData *shared;
    pid_t children[MAX_N];  // To store child PIDs.
    int i, j, status;

    signal(SIGINT, releaseSHM);  // Set up the signal handler to release shared memory on interrupt.

    // Create a shared memory segment.
    shmid = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach the shared memory segment to the address space of the process.
    shared = (SharedData *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat failed");
        releaseSHM(0); // Clean up shared memory before exiting.
        exit(1);
    }

    printf("Enter the dimensions of matrix A (n m): ");
    scanf("%d %d", &shared->n, &shared->m);

    printf("Enter the dimensions of matrix B (m p): ");
    scanf("%d %d", &i, &shared->p);

    if (i != shared->m) {
        printf("Error: Incompatible matrix dimensions (m values must match).\n");
        releaseSHM(0);  // Clean up shared memory.
        exit(1);
    }

    if (shared->n > MAX_N || shared->m > MAX_M || shared->p > MAX_P) {
        printf("Error: Matrix dimensions exceed maximum limits.\n");
        releaseSHM(0);
        exit(1);
    }

    printf("Enter the elements of matrix A:\n");
    for (i = 0; i < shared->n; i++) {
        for (j = 0; j < shared->m; j++) {
            scanf("%d", &shared->a[i][j]);
        }
    }

    printf("Enter the elements of matrix B:\n");
    for (i = 0; i < shared->m; i++) {
        for (j = 0; j < shared->p; j++) {
            scanf("%d", &shared->b[i][j]);
        }
    }

    // Create a child process for each row of the result matrix.
    for (i = 0; i < shared->n; i++) {
        children[i] = fork();
        if (children[i] == -1) {
            perror("fork failed");
            releaseSHM(0);
            exit(1);
        }

        if (children[i] == 0) { // This is the child process.
            calculate_row(shared, i);
            exit(0);
        }
    }

    // The parent process waits for all children to complete.
    for (i = 0; i < shared->n; i++) {
        waitpid(children[i], &status, 0);
    }

    // The parent process prints the result matrix.
    printf("Result matrix C:\n");
    for (i = 0; i < shared->n; i++) {
        for (j = 0; j < shared->p; j++) {
            printf("%d ", shared->c[i][j]);
        }
        printf("\n");
    }

    status = shmdt(shared); // Detach the shared memory segment.
    if (status == -1) {
        perror("shmdt failed");
    }
    releaseSHM(0); // Call the signal handler to properly remove the shared memory segment.
    return 0;
}
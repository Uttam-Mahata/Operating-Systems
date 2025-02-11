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

typedef struct {
    int a[MAX_N][MAX_M];
    int b[MAX_M][MAX_P];
    int c[MAX_N][MAX_P];
    int n, m, p;
} SharedData;

int shmid;

// Signal handler to release shared memory
void releaseSHM(int signum) {
    int status = shmctl(shmid, IPC_RMID, NULL);
    if (status == 0) {
        fprintf(stderr, "Removed shared memory with id = %d.\n", shmid);
    } else if (status == -1) {
        perror("Cannot remove shared memory");
    }

    kill(0, SIGKILL);  // Kill all processes
}

void calculate_row(SharedData *shared, int row_num) {
    for (int j = 0; j < shared->p; j++) {
        shared->c[row_num][j] = 0;
        for (int k = 0; k < shared->m; k++) {
            shared->c[row_num][j] += shared->a[row_num][k] * shared->b[k][j];
        }
    }
}

int main() {
    SharedData *shared;
    pid_t children[MAX_N];  // To Store child PIDs
    int i, j, status;

    signal(SIGINT, releaseSHM);  // Signal handler to release shared memory

    // Create shared memory segment
    shmid = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach shared memory
    shared = (SharedData *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat failed");
        releaseSHM(0); // Clean up shared memory before exiting
        exit(1);
    }

    printf("Enter the dimensions of matrix A (n m): ");
    scanf("%d %d", &shared->n, &shared->m);

    printf("Enter the dimensions of matrix B (m p): ");
    scanf("%d %d", &i, &shared->p);

    if (i != shared->m) {
        printf("Error: Incompatible matrix dimensions (m values must match).\n");
        releaseSHM(0);  // Clean up shared memory
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

    //Create child processes
    for (i = 0; i < shared->n; i++) {
        children[i] = fork();
        if (children[i] == -1) {
            perror("fork failed");
            releaseSHM(0);
            exit(1);
        }

        if (children[i] == 0) { // Child process
            calculate_row(shared, i);
            exit(0);
        }
    }

    // Parent process: wait for all children to complete
    for (i = 0; i < shared->n; i++) {
        waitpid(children[i], &status, 0);
    }

    // Parent process
    printf("Result matrix C:\n");
    for (i = 0; i < shared->n; i++) {
        for (j = 0; j < shared->p; j++) {
            printf("%d ", shared->c[i][j]);
        }
        printf("\n");
    }

    status = shmdt(shared); // Detach shared memory
    if (status == -1) {
        perror("shmdt failed");
    }
    releaseSHM(0); // Call  signal handler to properly remove shared memory
    return 0;
}
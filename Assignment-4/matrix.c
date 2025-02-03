#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define MAX_SIZE 10

// Structure to hold matrices and their dimensions
struct shared_data {
    int a[MAX_SIZE][MAX_SIZE];    // First matrix
    int b[MAX_SIZE][MAX_SIZE];    // Second matrix
    int c[MAX_SIZE][MAX_SIZE];    // Result matrix
    int n, m, p;                  // Matrix dimensions
    int completed_rows;           // Counter for completed rows
};

// Function to read matrix
void read_matrix(int matrix[][MAX_SIZE], int rows, int cols, char name) {
    printf("\nEnter elements for matrix %c (%dx%d):\n", name, rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%c[%d][%d]: ", name, i+1, j+1);
            scanf("%d", &matrix[i][j]);
        }
    }
}

// Function to print matrix
void print_matrix(int matrix[][MAX_SIZE], int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d\t", matrix[i][j]);
        }
        printf("\n");
    }
}

int main() {
    // Create shared memory
    key_t key = ftok(".", 'b');
    int shmid = shmget(key, sizeof(struct shared_data), IPC_CREAT | 0666);
    
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    
    // Attach shared memory
    struct shared_data *shared = (struct shared_data *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }
    
    // Get matrix dimensions
    printf("Enter dimensions:\n");
    printf("Matrix A (n x m) - Enter n: ");
    scanf("%d", &shared->n);
    printf("Enter m: ");
    scanf("%d", &shared->m);
    printf("Matrix B (%d x p) - Enter p: ", shared->m);
    scanf("%d", &shared->p);
    
    if (shared->n > MAX_SIZE || shared->m > MAX_SIZE || shared->p > MAX_SIZE) {
        printf("Error: Matrix dimensions exceed maximum size (%d)\n", MAX_SIZE);
        shmdt(shared);
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }
    
    // Read matrices
    read_matrix(shared->a, shared->n, shared->m, 'A');
    read_matrix(shared->b, shared->m, shared->p, 'B');
    
    shared->completed_rows = 0;
    
    // Create n child processes
    for (int i = 0; i < shared->n; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        }
        
        if (pid == 0) {  // Child process
            // Calculate one row of result matrix
            for (int j = 0; j < shared->p; j++) {
                shared->c[i][j] = 0;
                for (int k = 0; k < shared->m; k++) {
                    shared->c[i][j] += shared->a[i][k] * shared->b[k][j];
                }
            }
            
            // Increment completed rows counter
            shared->completed_rows++;
            
            // Detach shared memory and exit
            shmdt(shared);
            exit(0);
        }
    }
    
    // Parent process waits for all children to complete
    while (shared->completed_rows < shared->n) {
        usleep(100000);  // Sleep for 100ms
    }
    
    // Print the result matrix
    printf("\nResultant Matrix C (%dx%d):\n", shared->n, shared->p);
    print_matrix(shared->c, shared->n, shared->p);
    
    // Wait for all child processes
    for (int i = 0; i < shared->n; i++) {
        wait(NULL);
    }
    
    // Cleanup shared memory
    shmdt(shared);
    shmctl(shmid, IPC_RMID, NULL);
    printf("\nShared memory cleaned up\n");
    
    return 0;
}
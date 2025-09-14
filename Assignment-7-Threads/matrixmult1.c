/**
 * @file matrixmult1.c
 * @brief This program demonstrates matrix multiplication using multiple threads.
 * For the computation of every element of the result matrix C, one thread is created.
 * Since no two threads will be writing to the same element, no mutual exclusion is needed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

/* A is an m x n matrix, B is an n x r matrix, and C is an m x r matrix */
#define m 2
#define n 3
#define r 2

int A[m][n] = {{1, 2, 3}, {4, 5, 6}};
int B[n][r] = {{1, 2}, {3, 4}, {5, 6}};
int C[m][r];

/**
 * @struct v
 * @brief A structure to pass the row and column indices to a thread.
 */
struct v {
    int i; /**< Row index. */
    int j; /**< Column index. */
};

/**
 * @brief The function that is executed by each thread.
 * It calculates one element of the result matrix C.
 * @param param A pointer to a struct v containing the row and column indices.
 * @return NULL.
 */
void *runner(void *param);

/**
 * @brief The main function. It creates a thread for each element of the result matrix,
 * waits for all threads to complete, and then prints the result matrix.
 * @return 0 on success, 1 on failure.
 */
int main() {
    int i, j;
    int status;
    pthread_t tids[m * r];
    pthread_attr_t attr;

    // Set the default attributes for the threads.
    pthread_attr_init(&attr);

    // Create one thread for each element of the result matrix.
    for (i = 0; i < m; i++) {
        for (j = 0; j < r; j++) {
            struct v *data = (struct v *)malloc(sizeof(struct v));
            data->i = i;
            data->j = j;

            // Create the thread.
            status = pthread_create(&tids[i * r + j], &attr, runner, data);
            if (status != 0) {
                fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN ? "Insufficient resources" : (status == EINVAL ? "Invalid settings" : (status == EPERM ? "No permission" : "Unknown Error")));
                exit(1);
            }
        }
    }

    // Wait for all threads to complete.
    for (i = 0; i < m * r; i++) {
        status = pthread_join(tids[i], NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_join() failed: %s.\n", status == EDEADLK ? "Deadlock detected" : (status == EINVAL ? "Not a joinable thread" : (status == ESRCH ? "No such thread" : "Unknown Error")));
            exit(1);
        }
    }

    printf("Product of the matrices:\n");
    for (i = 0; i < m; i++) {
        for (j = 0; j < r; j++) {
            printf("%d\t", C[i][j]);
        }
        printf("\n");
    }

    return 0;
}

/**
 * @brief The function that is executed by each thread.
 * It calculates one element of the result matrix C.
 * @param param A pointer to a struct v containing the row and column indices.
 * @return NULL.
 */
void *runner(void *param) {
    struct v *data = (struct v *)param;
    int l, sum = 0;

    // Multiply the row of A by the column of B.
    for (l = 0; l < n; l++) {
        sum += A[data->i][l] * B[l][data->j];
    }

    // Assign the sum to the corresponding element of C.
    C[data->i][data->j] = sum;

    // Free the memory allocated for the parameter.
    free(param);

    // Exit the thread.
    pthread_exit(0);
}
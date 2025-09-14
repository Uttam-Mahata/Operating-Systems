/**
 * @file problem3.c
 * @brief This program demonstrates inter-process communication using the exit status of child processes.
 * The program creates child processes to find the k-th largest numbers in an array, and the parent process
 * collects these numbers from the child processes' exit statuses.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @brief Swaps the values of two integers.
 * @param xp Pointer to the first integer.
 * @param yp Pointer to the second integer.
 */
void swap(int* xp, int* yp) {
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

/**
 * @brief Sorts an array of integers in descending order using the bubble sort algorithm.
 * @param arr The array to be sorted.
 * @param n The size of the array.
 */
void bubbleSort(int arr[], int n) {
    int i, j;
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j] < arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
            }
        }
    }
}

/**
 * @brief The main function. It reads an array of integers, creates child processes to find the k-th largest numbers,
 * and then collects the results from the child processes' exit statuses.
 * @return 0 on success.
 */
int main() {
    int n;
    pid_t p;
    int *arr;
    int *sorted_arr;

    printf("Enter the size of the array: ");
    scanf("%d", &n);

    // Dynamically allocate memory for the arrays.
    arr = (int*) malloc(n * sizeof(int));
    sorted_arr = (int*) malloc(n * sizeof(int));

    printf("Enter the values for the array:\n");
    for (int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }

    // Create a child process for each element in the array.
    for (int i = 0; i < n; i++) {
        p = fork();

        if (p == -1) {
            printf("Fork Failed...\n");
            exit(1);
        }
        else if (p == 0) {
            // This is the child process.
            // The child process exits with the i-th largest number as its exit status.
            // Note: This only works for small integers (0-255).
            exit(arr[n - i - 1]);
        }
    }

    // This is the parent process.
    // The parent process waits for each child process to terminate and collects its exit status.
    for (int i = 0; i < n; i++) {
        int status;
        wait(&status); // Wait for any child process to finish.

        // Check if the child process terminated normally.
        if (WIFEXITED(status)) {
            // Get the exit status of the child process.
            sorted_arr[i] = WEXITSTATUS(status);
        }
    }

    // Sort the array of numbers collected from the child processes.
    bubbleSort(sorted_arr, n);

    printf("\nThe numbers in descending order are:\n");
    for (int i = 0; i < n; i++) {
        printf("%d ", sorted_arr[i]);
    }
    printf("\n");

    // Free the dynamically allocated memory.
    free(arr);
    free(sorted_arr);

    return 0;
}

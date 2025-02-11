#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void swap(int* xp, int* yp) {
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// Bubble Sort to sort the array in descending order
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

int main() {
    int n;
    pid_t p;
    int *arr;
    int *sorted_arr;
    
    printf("Enter the size of the array: ");
    scanf("%d", &n);
    
    // Dynamic allocation of memory for the array
    arr = (int*) malloc(n * sizeof(int));
    sorted_arr = (int*) malloc(n * sizeof(int));

    printf("Enter the values for the array:\n");
    for (int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }

    // Creation of child processes
    for (int i = 0; i < n; i++) {
        p = fork();

        if (p == -1) {
            printf("Fork Failed...\n");
            exit(1);
        }
        else if (p == 0) {
            exit(arr[n - i - 1]);  // Passing the i-th largest number to parent via exit status
        }
    }

    // Parent process
    for (int i = 0; i < n; i++) {
        int status;
        pid_t child_pid = wait(&status); // Wait for any child process to finish

        if (WIFEXITED(status)) {
            sorted_arr[i] = WEXITSTATUS(status);
        }
    }

    bubbleSort(sorted_arr, n);

    printf("\nThe numbers in descending order are:\n");
    for (int i = 0; i < n; i++) {
        printf("%d ", sorted_arr[i]);
    }
    printf("\n");

    free(arr);
    free(sorted_arr);

    return 0;
}

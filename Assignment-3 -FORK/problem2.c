/**
 * @file problem2.c
 * @brief This program demonstrates the use of the fork() system call to find the k-th largest numbers in an array.
 * The program first sorts the array, then creates a child process for each element to print the k-th largest number.
 */

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/types.h>

/**
 * @brief Swaps the values of two integers.
 * @param xp Pointer to the first integer.
 * @param yp Pointer to the second integer.
 */
void swap(int* xp, int* yp){
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

/**
 * @brief Sorts an array of integers using the bubble sort algorithm.
 * @param arr The array to be sorted.
 * @param n The size of the array.
 */
void bubbleSort(int arr[], int n){
    int i, j;
    bool swapped;
    for (i = 0; i < n - 1; i++) {
        swapped = false;
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
                swapped = true;
            }
        }

        // If no two elements were swapped by inner loop, then the array is sorted.
        if (swapped == false)
            break;
    }
}

/**
 * @brief The main function. It reads an array of integers, sorts it, and then creates a child process for each element to print the k-th largest number.
 * @return 0 on success.
 */
int main() {
    int n;
    pid_t p;

    printf("Enter the size of the array: ");
    scanf("%d", &n);

    int arr[n];

    printf("Enter the values for the array:\n");

    for(int i = 0; i < n; i++) {
        int x;
        scanf("%d", &x);
        arr[i] = x;
    }

    // Sort the array in ascending order.
    bubbleSort(arr, n);

    // Create a child process for each element in the array.
    for(int i = 0; i < n; i++) {
        p = fork();

        if(p == -1) {
            printf("Fork Failed...");
            exit(1);
        }
        else if(p == 0) {
            // This is the child process.
            printf("Child Process %d! \n",i+1);
            // The k-th largest number is at index n-i-1 in the sorted array.
            printf("%d -th largest number is: %d\n",i+1,arr[n-i-1]);
            exit(0);
        }
    }

    return 0;
}

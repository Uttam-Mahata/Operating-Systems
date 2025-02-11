#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/types.h>

void swap(int* xp, int* yp){
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// Bubble Sort
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

        // If no two elements were swapped by inner loop,
        // then break
        if (swapped == false)
            break;
    }
}

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

bubbleSort(arr, n);

for(int i = 0; i < n; i++) {
p = fork();

if(p == -1) {
printf("Fork Failed...");
exit(0);
}
else if(p == 0) {
printf("Child Process %d! \n",i+1);
printf("%d -th largest number is: %d",i+1,arr[n-i-1]);
exit(9);
}
}

return 0;
}

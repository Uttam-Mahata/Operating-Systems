/**
 * @file problem1.c
 * @brief This program demonstrates the use of the fork() system call to create multiple child processes.
 * Each child process is responsible for reversing one of the command-line arguments.
 */

#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>

/**
 * @brief Reverses a string and prints it to the console.
 * @param str The string to be reversed.
 */
void reverse_string(char* str) {
    int len=strlen(str);
    for(int i = len -1 ; i >=0; i--) {
        printf("%c", str[i]);
    }
    printf("\n");
}

/**
 * @brief The main function. It creates a child process for each command-line argument.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success.
 */
int main(int argc, char *argv[]) {
    int i;
    pid_t p;
    // The number of strings to reverse is the number of arguments minus the program name.
    int n = argc -1;

    // Create a new process for each command-line argument.
    for(i = 0; i <n; i++) {
        p = fork();

        // If fork() returns -1, an error occurred.
        if(p == -1) {
            printf("Fork Failed....");
            exit(1); // Exit with a non-zero status to indicate an error.
        }

        // If fork() returns 0, we are in the child process.
        if(p == 0) {
            printf("Child Process %d\n", i+1);
            // The child process reverses the string and then exits.
            reverse_string(argv[i+1]);
            exit(0); // Exit with a status of 0 to indicate success.
        } else {
            // If fork() returns a positive value, we are in the parent process.
            // The parent process simply prints a message and continues to the next iteration of the loop.
            printf("Parent Process\n");
        }
    }

    return 0;
}

/**
 * @file sample_program4.c
 * @brief A program to demonstrate creating multiple child processes in a loop.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

/**
 * @brief The main function. It creates 5 child processes in a loop.
 * @return 0 on success.
 */
int main() {
    pid_t p;
    int i;

    // This loop will create 5 child processes.
    for (i = 0; i < 5; i++) {
        p = fork();

        if (p == 0) {
            // This is the child process.
            printf("Child process %d !\n", i + 1);
            // The child process exits after printing its message.
            // This is important to prevent the child from continuing the loop and creating its own children.
            exit(0);
        } else {
            // This is the parent process.
            printf("Parent process!\n");
        }
    }

    return 0;
}

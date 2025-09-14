/**
 * @file sample_program2.c
 * @brief A simple program to demonstrate how to differentiate between the parent and child process after a fork().
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

/**
 * @brief The main function. It forks a process and then prints messages to indicate whether the process is the parent or the child.
 * @return 0 on success.
 */
int main() {
    pid_t p;

    // This message is printed only once, by the parent process.
    printf("Hello World\n");

    p = fork();

    if(p == -1) {
        printf("Fork Failed\n");
    }

    /**
     * The return value of fork() is used to determine whether the current process is the parent or the child.
     * - In the parent process, fork() returns the process ID (PID) of the child process.
     * - In the child process, fork() returns 0.
     */
    if(p == 0) {
        // This code is executed by the child process.
        printf("Child Process\n");
    } else {
        // This code is executed by the parent process.
        printf("Parent Process\n");
    }

    // This message is printed by both the parent and the child process.
    printf("Executed by both Parent and Child Process\n");

    return 0;
}

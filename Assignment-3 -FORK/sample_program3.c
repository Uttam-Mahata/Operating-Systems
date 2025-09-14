/**
 * @file sample_program3.c
 * @brief A simple program to demonstrate that the child process gets a copy of the parent's variables.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

/**
 * @brief The main function. It initializes two variables, forks a process, and then performs different calculations in the parent and child processes.
 * @return 0 on success.
 */
int main() {
    int a = 3;
    int b = 5;
    pid_t p;

    p = fork();

    if (p == 0) {
        // This is the child process. It has its own copies of 'a' and 'b'.
        int c = a + b;
        printf("Value of c (by child process) : %d\n", c);
    } else {
        // This is the parent process.
        int d = a * b;
        printf("Value of d (by parent process) : %d\n", d);
    }

    // This code is executed by both the parent and child processes.
    int e = b - a;
    printf("Value of e (by parent and child process) : %d\n", e);

    return 0;
}

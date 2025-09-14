/**
 * @file forkNexec2.c
 * @brief This program is an enhanced version of forkNexec1.c. It demonstrates a basic use of the fork() and wait() system calls,
 * with more descriptive output from the parent and child processes.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @brief The main function. It runs in an infinite loop, reading a string, forking a child process to print the string,
 * and then waiting for the child to terminate.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success.
 */
int main(int argc, char *argv[]) {
    int status;

    while (1) {
        char str[50];

        printf("Enter a string: ");
        scanf("%s", str);

        pid_t p = fork();

        if (p == -1) {
            perror("fork() failed");
            exit(1);
        }

        // This is the child process.
        if (p == 0) {
            printf("Entered string is(by Child Process): %s\n", str);
            // The child process exits with a status of 9.
            exit(9);
        }

        // This is the parent process.
        // The parent waits for the child to terminate.
        wait(&status);

        // The parent prints the exit status of the child.
        printf("Parent: Child exited with status %d\n", WEXITSTATUS(status));
    }

    return 0;
}
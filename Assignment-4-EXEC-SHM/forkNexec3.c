/**
 * @file forkNexec3.c
 * @brief This program demonstrates the use of the fork() and execve() system calls to run another program.
 * The parent process reads an executable file name from the user, creates a child process, and the child process
 * executes the specified program. The parent process waits for the child to terminate and prints its exit status.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @brief The main function. It runs in an infinite loop, reading a file name, forking a child process to execute the file,
 * and then waiting for the child to terminate.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success.
 */
int main(int argc, char *argv[]) {
    int status;

    while (1) {
        char str[50];

        printf("Enter the executable file name: ");
        scanf("%s", str);

        pid_t p = fork();

        if (p == -1) {
            perror("fork() failed");
            exit(1);
        }

        // This is the child process.
        if (p == 0) {
            char *myargv[] = {str, NULL};
            // The execve() system call replaces the current process image with a new one.
            // If execve() is successful, it does not return.
            execve(str, myargv, NULL);

            // This code will only be executed if execve() fails.
            perror("Exec Fails: ");
            exit(9);
        } else {
            // This is the parent process.
            // The parent waits for the child to terminate.
            wait(&status);

            // Check if the child was terminated by a signal.
            if (WIFSIGNALED(status)) {
                printf("Child process was terminated by signal %d\n", WTERMSIG(status));
            } else {
                // The parent prints the exit status of the child.
                printf("Parent: Child exited with status %d\n", WEXITSTATUS(status));
            }
        }
    }
    return 0;
}
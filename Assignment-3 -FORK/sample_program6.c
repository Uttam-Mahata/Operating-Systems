/**
 * @file sample_program6.c
 * @brief A program to demonstrate the use of the wait() system call to get the exit status of a child process.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @brief The main function. It forks a child process, which does some work and then exits with a specific status code.
 * The parent process waits for the child to terminate and then prints its exit status.
 * @return 0 on success.
 */
int main() {
    int status;
    pid_t pid = 0;
    pid_t p1 = 0;

    printf("Hello World!\n");

    p1 = fork();

    if (p1 == 0) {
        // This part is executed by the child process.
        int i;

        // The following loop is just to keep the child executing something
        // so that it is live for some period and does not terminate immediately.
        for (i = 0; i < 5; i++) {
            printf("%d\n", i++);
            // The child process waits for user input, which keeps it alive.
            getchar();
        }

        // The child process terminates with status 12.
        // This status is communicated to the parent process.
        exit(12);
    }

    // This part will be executed only by the parent process.
    // The wait() system call suspends the execution of the parent process until one of its children terminates.
    pid = wait(&status);

    // The status variable contains information about the child's termination.
    // To get the actual exit code, you need to use the WEXITSTATUS() macro.
    printf("pid = %d status = %d!\n", pid, status);
    if (WIFEXITED(status)) {
        printf("Child exited with status %d\n", WEXITSTATUS(status));
    }

    return 0;
}
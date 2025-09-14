/**
 * @file forkNexec4.c
 * @brief This program implements a simple shell that can execute commands with arguments.
 * It demonstrates the use of fork(), execvp(), and wait() to create a new process to execute a command.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define MAX_ARGS 16

/**
 * @brief The main function. It runs in an infinite loop, reading a command and its arguments from the user,
 * forking a child process to execute the command, and then waiting for the child to terminate.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success.
 */
int main(int argc, char *argv[]) {
    int status;
    char line[MAX_LINE_LENGTH];

    while (1) {
        printf("Enter command and arguments: ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            perror("fgets error");
            break;
        }

        // Remove trailing newline character, if present.
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // Tokenize the input line into a command and its arguments.
        char *args[MAX_ARGS + 1];
        char *token = strtok(line, " "); // Tokenize by space.
        int i = 0;

        while (token != NULL && i < MAX_ARGS) {
            args[i] = token;
            token = strtok(NULL, " ");
            i++;
        }

        args[i] = NULL; // Null-terminate the argument list.

        if (args[0] == NULL) {
            continue; // Ignore empty lines.
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork failed");
            continue;
        }

        if (pid == 0) {
            // This is the child process.
            // execvp() searches for the command in the directories specified in the PATH environment variable.
            execvp(args[0], args);
            // If execvp() returns, an error has occurred.
            perror("execvp failed");
            exit(99);
        } else {
            // This is the parent process.
            pid_t wpid = wait(&status);

            if (wpid == -1) {
                perror("wait failed");
                continue;
            }

            if (WIFEXITED(status)) {
                printf("Child process exited with status %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Child process was terminated by signal %d\n", WTERMSIG(status));
            }
        }
    }

    return 0;
}
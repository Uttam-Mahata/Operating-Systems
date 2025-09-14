/**
 * @file sample_program5.c
 * @brief A simple program to demonstrate the use of the execve() system call.
 * This program replaces the current process image with a new process image.
 */

#include <stdio.h> /* needed also for perror() */
#include <errno.h> /* needed  for perror() */
#include <unistd.h> /* needed for execve() */

/**
 * @brief The main function. It uses execve() to execute the /bin/ls command.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return This function does not return on success. On failure, it returns -1.
 */
int main(int argc, char *argv[]) {
    int status;
    // The first argument is the program to execute, and the rest are its arguments.
    // The list must be null-terminated.
    char *myargv[] = {"/bin/ls", NULL};

    // The execve() system call replaces the current process with a new one.
    // The new process is the one specified in the first argument.
    // If execve() is successful, it does not return.
    status = execve("/bin/ls", myargv, NULL);

    // This code will only be executed if execve() fails.
    if (status == -1) {
        perror("Exec Fails: ");
    }

    // This return statement will not be reached if execve() is successful.
    return 1;
}
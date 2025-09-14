/**
 * @file sample_program.c
 * @brief A simple program to demonstrate the effect of the fork() system call.
 */

#include<stdio.h>
#include<unistd.h>

/**
 * @brief The main function. It prints a message, forks the process, and then prints another message.
 * @return 0 on success.
 */
int main() {
    // This message is printed only once, by the parent process.
    printf("Hello World - by Parent Process\n");
    // The fork() system call creates a new process.
    // The new process (the child) is an exact copy of the parent.
    fork();
    // This message is printed twice: once by the parent and once by the child.
    printf("Hello Uttam - by Parent and Child Process\n");
    return 0;
}

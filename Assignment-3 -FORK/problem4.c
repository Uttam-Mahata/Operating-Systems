/**
 * @file problem4.c
 * @brief This program demonstrates file sharing between a parent and child process.
 * It shows how the file position indicator is shared after a fork() and how closing the file in one process
 * does not affect the other.
 *
 * This program answers the following questions:
 * 1. Can the child process read from the file using the file pointer fp?
 *    Yes, the child process (P2) can read from the file using the file pointer fp.
 *    This is because fork() creates a duplicate of the file descriptor table, and the file pointer is valid in both processes.
 * 2. Where will the child process start reading from?
 *    P2 will start reading from the same file position where P1 left off before the fork(). This is because the file offset is also duplicated during fork().
 * 3. How do the file position indicators behave?
 *    Both processes maintain their own file position indicators. When either process reads from the file, it advances its own file position independently.
 *    Parent reads first line before fork. After fork, both processes have their file pointers at the beginning of line 2.
 *    When they read subsequently, they each get different lines because they maintain separate file positions.
 * 4. Does closing the file in the parent affect the child?
 *    No, when P1 closes the file, it remains open for P2. This is because after fork():
 *    - Each process has its own file descriptor table.
 *    - The close operation in one process doesn't affect the other.
 *    - Each process must close its own file descriptor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

/**
 * @brief Reads a line from a file and prints it to the console, along with the file position before and after the read.
 * @param fp The file pointer to read from.
 * @param process_name The name of the process that is reading from the file.
 */
void read_and_print(FILE *fp, const char *process_name) {
    char buffer[100];

    printf("\n%s - File position before read: %ld\n",
           process_name, ftell(fp));

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("%s read: %s", process_name, buffer);
    } else {
        printf("%s reached EOF\n", process_name);
    }

    printf("%s - File position after read: %ld\n",
           process_name, ftell(fp));
}

/**
 * @brief The main function. It creates a file, writes some data to it, and then forks a child process.
 * Both the parent and child processes read from the file to demonstrate file sharing.
 * @return 0 on success, 1 on failure.
 */
int main() {
    // Create a temporary file for the demonstration.
    FILE *temp = fopen("oslab.txt", "w");
    fprintf(temp, "Line 1 - This is a test file\n");
    fprintf(temp, "Line 2 - Testing file sharing between processes\n");
    fprintf(temp, "Line 3 - Final line of the file\n");
    fclose(temp);

    // Open the file for reading.
    FILE *fp = fopen("oslab.txt", "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    printf("Initial file position: %ld\n", ftell(fp));

    // The parent process reads the first line before forking.
    printf("\nParent reading before fork:\n");
    read_and_print(fp, "Parent");

    // Fork the process.
    pid_t p = fork();

    if (p == -1) {
        perror("Fork failed");
        exit(1);
    }

    if (p == 0) {
        // This is the child process.
        printf("\nChild process starting - PID: %d\n", getpid());

        // The child reads from the file. It will start where the parent left off.
        read_and_print(fp, "Child");

        // The child reads again.
        read_and_print(fp, "Child");

        // The child waits for a moment to see if the parent closing the file has any effect.
        sleep(2);
        printf("\nChild attempting to read after delay:\n");
        read_and_print(fp, "Child");

        fclose(fp);
        exit(0);

    } else {
        // This is the parent process.
        printf("\nParent process continuing - PID: %d\n", getpid());

        // The parent reads from the file.
        read_and_print(fp, "Parent");

        // The parent closes the file.
        printf("\nParent closing the file\n");
        fclose(fp);

        // The parent waits for the child to finish.
        wait(NULL);
    }

    return 0;
}
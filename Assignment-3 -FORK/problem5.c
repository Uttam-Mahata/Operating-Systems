/**
 * @file problem5.c
 * @brief This program demonstrates concurrent file writing by a parent and child process.
 * It shows how the file position indicator is shared and how writes can be interleaved.
 *
 * This program answers the following questions:
 * 1. Can the child process write to the file?
 *    Yes, the child process (P2) can write to the file using the duplicated file pointer fp.
 *    The file descriptor is copied during fork(), giving the child process its own valid file handle.
 * 2. Where will the child process start writing?
 *    P2 will write at the file position where P1 left off before the fork().
 *    The file offset is shared initially and then becomes independent for each process. Each write operation advances that process's file position.
 * 3. What is the result of concurrent writing?
 *    Both processes can write to the file, and their writes will be interleaved based on:
 *    - Their relative execution timing
 *    - The OS scheduling
 *    - The buffering and flushing of the file stream
 * 4. Does closing the file in the parent affect the child?
 *    No, when P1 closes the file, it remains open for P2. This is because:
 *    - Each process has its own file descriptor table after fork()
 *    - The close operation in one process only affects that process's file descriptor
 *    - P2 can continue writing even after P1 closes the file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

/**
 * @brief Writes a message to a file and prints the file position before and after the write.
 * @param fp The file pointer to write to.
 * @param process_name The name of the process writing to the file.
 * @param message The message to write.
 */
void write_and_show_pos(FILE *fp, const char *process_name, const char *message) {
    long pos_before = ftell(fp);
    printf("%s - Position before write: %ld\n", process_name, pos_before);

    fprintf(fp, "[%s writes: %s]\n", process_name, message);
    fflush(fp);  // Force write to disk to see the interleaving.

    long pos_after = ftell(fp);
    printf("%s - Position after write: %ld\n", process_name, pos_after);
}

/**
 * @brief Displays the contents of a file.
 * @param filename The name of the file to display.
 */
void display_file_contents(const char *filename) {
    printf("\nFile contents:\n------------\n");
    FILE *fp = fopen(filename, "r");
    if (fp != NULL) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            printf("%s", buffer);
        }
        fclose(fp);
    }
    printf("-----------\n");
}

/**
 * @brief The main function. It opens a file for writing, forks a child process, and then both processes write to the file.
 * @return 0 on success, 1 on failure.
 */
int main() {
    const char *filename = "hello.txt";

    // Open file for writing. This will create the file if it doesn't exist, or truncate it if it does.
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    printf("Initial file position: %ld\n\n", ftell(fp));

    // Parent writes the first message before forking.
    write_and_show_pos(fp, "Parent", "Message before fork");

    // Fork the process.
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }

    if (pid == 0) {
        // This is the child process.
        printf("\nChild process (PID: %d) starting\n", getpid());

        // Child writes multiple messages.
        write_and_show_pos(fp, "Child", "First message");
        sleep(1);
        write_and_show_pos(fp, "Child", "Second message");

        // Child waits for a moment to see if the parent closing the file has any effect.
        sleep(2);
        printf("\nChild attempting to write after delay:\n");
        write_and_show_pos(fp, "Child", "Message after parent might have closed");

        fclose(fp);
        exit(0);

    } else {
        // This is the parent process.
        printf("\nParent process (PID: %d) continuing\n", getpid());

        // Parent writes its messages.
        sleep(1);
        write_and_show_pos(fp, "Parent", "Message after fork");

        printf("\nParent closing the file\n");
        fclose(fp);

        // Parent waits for the child to finish.
        wait(NULL);

        // Display the final contents of the file.
        display_file_contents(filename);
    }

    return 0;
}
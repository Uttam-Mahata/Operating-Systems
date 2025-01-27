#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void write_and_show_pos(FILE *fp, const char *process_name, const char *message) {
    long pos_before = ftell(fp);
    printf("%s - Position before write: %ld\n", process_name, pos_before);
    
    fprintf(fp, "[%s writes: %s]\n", process_name, message);
    fflush(fp);  // Force write to disk
    
    long pos_after = ftell(fp);
    printf("%s - Position after write: %ld\n", process_name, pos_after);
}

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

int main() {
    const char *filename = "hello.txt";
    
    // Open file for writing
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }
    
    printf("Initial file position: %ld\n\n", ftell(fp));
    
    // Parent writes first message before fork
    write_and_show_pos(fp, "Parent", "Message before fork");
    
    // Fork the process
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }
    
    if (pid == 0) {
        printf("\nChild process (PID: %d) starting\n", getpid());
        
        // Child writes multiple messages
        write_and_show_pos(fp, "Child", "First message");
        sleep(1);
        write_and_show_pos(fp, "Child", "Second message");
        
        // Child tries to write after parent might have closed
        sleep(2);
        printf("\nChild attempting to write after delay:\n");
        write_and_show_pos(fp, "Child", "Message after parent might have closed");
        
        fclose(fp);
        exit(0);
        
    } else {
        printf("\nParent process (PID: %d) continuing\n", getpid());
        
        // Parent writes its messages
        sleep(1);
        write_and_show_pos(fp, "Parent", "Message after fork");
        
        printf("\nParent closing the file\n");
        fclose(fp);
        
        wait(NULL);
        
        display_file_contents(filename);
    }
    
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void read_and_print(FILE *fp, const char *process_name) {
    char buffer[100];
    int bytes_read;
    
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

int main() {
    FILE *temp = fopen("oslab.txt", "w");
    fprintf(temp, "Line 1 - This is a test file\n");
    fprintf(temp, "Line 2 - Testing file sharing between processes\n");
    fprintf(temp, "Line 3 - Final line of the file\n");
    fclose(temp);
    
    // Opening file for reading
    FILE *fp = fopen("oslab.txt", "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }
    
    printf("Initial file position: %ld\n", ftell(fp));
    
    // Reading first line in parent before fork
    printf("\nParent reading before fork:\n");
    read_and_print(fp, "Parent");
    
    // Fork the process
    pid_t p = fork();
    
    if (p == -1) {
        perror("Fork failed");
        exit(1);
    }
    
    if (p == 0) {
        printf("\nChild process starting - PID: %d\n", getpid());
        
        // Child reads from file
        read_and_print(fp, "Child");
        
        // Child reads again
        read_and_print(fp, "Child");
        
        // Child tries to read after parent might have closed
        sleep(2);
        printf("\nChild attempting to read after delay:\n");
        read_and_print(fp, "Child");
        
        fclose(fp);
        exit(0);
        
    } else {
        printf("\nParent process continuing - PID: %d\n", getpid());
        
        // Parent reads from file
        read_and_print(fp, "Parent");
        
        // Parent closes the file
        printf("\nParent closing the file\n");
        fclose(fp);
        
        wait(NULL);
    }
    
    return 0;
}
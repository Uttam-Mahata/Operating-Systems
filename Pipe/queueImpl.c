/*Queue Implementation using pipe() system call*/

/*
       #include <unistd.h>

       int pipe(int pipefd[2]);

pipe()  creates  a pipe, a unidirectional data channel that can be used
       for interprocess communication.  The array pipefd is used to return two
       file descriptors referring to the ends of the pipe.   pipefd[0]  refers
       to  the read end of the pipe.  pipefd[1] refers to the write end of the
       pipe.  Data written to the write end of the pipe  is  buffered  by  the
       kernel until it is read from the read end of the pipe.
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>

#define BUFFER_SIZE 256

void createQ(int pipefd[2]) {
    int flag;
    flag = pipe(pipefd);
    if (flag == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    printf("Pipe created successfully\n");
}

void enQ(int pipefd[2], char *data) {
    int flag;
    size_t len = strlen(data) + 1;  // Include null terminator
    
    // First write the length of the data
    flag = write(pipefd[1], &len, sizeof(size_t));
    if (flag == -1) {
        perror("write length");
        exit(EXIT_FAILURE);
    }
    
    // Then write the actual data
flag = write(pipefd[1], data, len);
    if (flag == -1) {
        perror("write data");
        exit(EXIT_FAILURE);
    }



    
    printf("Enqueued: %s\n", data);
}

void deQ(int pipefd[2], char *buffer) {
    size_t len;
    int flag;

    // First read the length of the data
    ssize_t read_len = read(pipefd[0], &len, sizeof(size_t));
    if (read_len == -1) {
        perror("read length");
        exit(EXIT_FAILURE);
    }
    
    if (read_len == 0) {
        // End of file
        buffer[0] = '\0';
        return;
    }
    
    // Then read the actual data
    flag = read(pipefd[0], buffer, len);

    if (flag == -1) {
        perror("read data");
        exit(EXIT_FAILURE);
    }

}

void enQInt(int pipefd[2], int data) {
    int flag;
    flag = write(pipefd[1], &data, sizeof(int));
    if (flag == -1) {
        perror("write int");
        exit(EXIT_FAILURE);
    }
    printf("Enqueued int: %d\n", data);
}

void deQInt(int pipefd[2], int *data) {
    int flag;
    flag = read(pipefd[0], data, sizeof(int));
    if (flag == -1) {
        perror("read int");
        exit(EXIT_FAILURE);
    }
    else if (flag == 0) {
        // End of file reached
        *data = 0;  // Use 0 to signal EOF
        return;
    }
    printf("Dequeued int: %d\n", *data);
}

int main(int argc, char *argv[]) {
    int pipefd[2];

    createQ(pipefd);

    char* data1 = "Hi!Writer.";
    char* data2 = "Hi!Reader.";

    int x = 3;
    int y = 4;
    int z = x + y;

    enQInt(pipefd, x);
    enQInt(pipefd, y);
    enQInt(pipefd, z);

    // Close write end before reading to signal EOF
    close(pipefd[1]);

    int value;
    int count = 0;
    
    // Read each of the 3 integers we've written
    while(count < 3) {
        deQInt(pipefd, &value);
        if (value == 0) {
            // End of file
            break;
        }
        count++;
    }
    
    // Close read end when done
    close(pipefd[0]);
    
    return 0;
}
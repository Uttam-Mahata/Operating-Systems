/* Storing Multiple Files in a Single Linux File*/
/*1st 8 blocks (8*4096 bytes) of dd1 contains metadata(name,size)
Remaining 2048 blocks of dd1 are data blocks of the contained files.
12 bytes for name
4 bytes for size
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

# define METADATA_BLOCK 8
#define DATA_BLOCK 2048 

#define BLOCK_SIZE 4096

#define MAX_FILE_NAME 12 /*File name size max 12 bytes*/
#define FILE_SIZE 4  /*Size of file in metadata*/

struct file_metadata {
    char name[MAX_FILE_NAME]; /*File name*/
    int size; /*Size of file*/
} FileMetadata;

int read_block(int fd, int block_num, char *buffer) {
    lseek(fd, block_num * BLOCK_SIZE, SEEK_SET);
    return read(fd, buffer, BLOCK_SIZE);
}
int write_block(int fd, int block_num, char *buffer) {
    lseek(fd, block_num * BLOCK_SIZE, SEEK_SET);
    return write(fd, buffer, BLOCK_SIZE);
}
int create_filesystem(char *filename) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Error creating filesystem");
        return -1;
    }
    char buffer[BLOCK_SIZE] = {0};
    for (int i = 0; i < METADATA_BLOCK + DATA_BLOCK; i++) {
        write_block(fd, i, buffer);
    }
    close(fd);
    return 0;
}

int get_free_block(char *filename) {
    int fd = open
    (filename, O_RDWR);
    if (fd < 0) {
        perror("Error opening filesystem");
        return -1;
    }
    char buffer[BLOCK_SIZE];
    for (int i = 0; i < DATA_BLOCK; i++) {
        read_block(fd, METADATA_BLOCK + i, buffer);
        if (buffer[0] == '\0') { // Check if the block is free
            close(fd);
            return METADATA_BLOCK + i;
        }
    }
    close(fd);
    return -1; // No free block found
}
int write_metadata(char *filename, int block_num, char *name, int size) {
    int fd = open(filename,O_RDWR);
    if (fd < 0) {
        perror("Error opening filesystem");
        return -1;
    }
    char buffer[BLOCK_SIZE];
    lseek(fd, block_num * BLOCK_SIZE, SEEK_SET);
    struct file_metadata metadata;
    strncpy(metadata.name, name, MAX_FILE_NAME);

    metadata.size = size;
    memcpy(buffer, &metadata, sizeof(struct file_metadata));
    write(fd, buffer, sizeof(struct file_metadata));
    close(fd);
    return 0;
}

int read_metadata(char *filename, int block_num, struct file_metadata *metadata) {
    int fd = open(filename, O_RDONLY); 
    if (fd < 0) {
        perror("Error opening filesystem");
        return -1;
    } 
    char buffer[BLOCK_SIZE];
    lseek(fd, block_num * BLOCK_SIZE, SEEK_SET);
    read(fd, buffer, sizeof(struct file_metadata));
    memcpy(metadata, buffer, sizeof(struct file_metadata));
    close(fd);
    return 0;
}

void mymkfs(char *filename) {
    if (create_filesystem(filename) == 0) {
        printf("Filesystem %s created successfully.\n", filename);
    } else {
        printf("Failed to create filesystem %s.\n", filename);
    }
}

void myCopyTo(char *filename, char *linux_file) {
    int fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("Error opening filesystem");
        return;
    }
    int block_num = get_free_block(filename);
    if (block_num < 0) {
        printf("No free block available in filesystem.\n");
        close(fd);
        return;
    }
    int linux_fd = open(linux_file, O_RDONLY);
    if (linux_fd < 0) {
        perror("Error opening Linux file");
        close(fd);
        return;
    }
    char buffer[BLOCK_SIZE];
    struct file_metadata metadata;
    read_metadata(filename, block_num, &metadata);
    int bytes_read = read(linux_fd, buffer, BLOCK_SIZE);
    if (bytes_read < 0) {
        perror("Error reading Linux file");
        close(linux_fd);
        close(fd);
        return;
    }
    write_block(fd, block_num, buffer);
    metadata.size = bytes_read;
    strncpy(metadata.name, linux_file, MAX_FILE_NAME);
    write_metadata(filename, block_num, metadata.name, metadata.size);
    printf("File %s copied to filesystem %s at block %d.\n", linux_file, filename, block_num);
    close(linux_fd);
    close(fd);
}

void myCopyFrom(char *filename, char *linux_file) {
    int fd = open
    (filename, O_RDWR);
    if (fd < 0) {
        perror("Error opening filesystem");
        return;
    }
    struct file_metadata metadata;
    char buffer[BLOCK_SIZE];
    int block_num = -1;
    for (int i = 0; i < DATA_BLOCK; i++) {
        read_metadata(filename, METADATA_BLOCK + i, &metadata);
        if (strcmp(metadata.name, linux_file) == 0) {
            block_num = METADATA_BLOCK + i;
            break;
        }
    }
    if (block_num < 0) {
        printf("File %s not found in filesystem.\n", linux_file);
        close(fd);
        return;
    }
    read_block(fd, block_num, buffer);
    int linux_fd = open(linux_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (linux_fd < 0) {
        perror("Error opening Linux file");
        close(fd);
        return;
    }
    write(linux_fd, buffer, metadata.size);
    printf("File %s copied from filesystem %s to Linux file %s.\n", linux_file, filename, linux_file);
    close(linux_fd);
    close(fd);
}

void myrm(char *filename, char *linux_file) {
    int fd = open
    (filename, O_RDWR);
    if (fd < 0) {
        perror("Error opening filesystem");
        return;
    }
    struct file_metadata metadata;
    int block_num = -1;
    for (int i = 0; i < DATA_BLOCK; i++) {
        read_metadata(filename, METADATA_BLOCK + i, &metadata);
        if (strcmp(metadata.name, linux_file) == 0) {
            block_num = METADATA_BLOCK + i;
            break;
        }
    }
    if (block_num < 0) {
        printf("File %s not found in filesystem.\n", linux_file);
        close(fd);
        return;
    }
    char buffer[BLOCK_SIZE] = {0};
    write_block(fd, block_num, buffer);
    printf("File %s removed from filesystem %s.\n", linux_file, filename);
    close(fd);

}

int main() {

/* Interactive menu for user to choose options

1. mymkfs dd1 [makes dd1 ready for storing files]
2. mycopyTo <linux file> dd1 [copies a linux file to dd1]
3. mycopyFrom <file name>@dd1 [copies a file from dd1 to a linux file of same name.]
4.  myrm <file name>@dd1 [removes a file from dd1]

*/
    char command[256];
    char filename[256];
    char linux_file[256];
    while (1) {
        printf("Enter command: ");
        fgets(command, sizeof(command), stdin);
        if (sscanf(command, "mymkfs %s", filename) == 1) {
            mymkfs(filename);
        } else if (sscanf(command, "mycopyTo %s %s", linux_file, filename) == 2) {
            myCopyTo(filename, linux_file);
        } else if (sscanf(command, "mycopyFrom %s %s", filename, linux_file) == 2) {
            myCopyFrom(filename, linux_file);
        } else if (sscanf(command, "myrm %s %s", filename, linux_file) == 2) {
            myrm(filename, linux_file);
        } else {
            printf("Invalid command.\n");
        }
    }
    return 0;

}

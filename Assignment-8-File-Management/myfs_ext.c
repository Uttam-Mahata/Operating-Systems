/**
 * @file myfs_ext.c
 * @brief An extended version of a simple file system implementation.
 * This version adds support for folders and block chaining to allow for files larger than a single block.
 * @note This implementation is incomplete.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SUPERBLOCK_SIZE 4096
#define SIZE 12

/**
 * @struct superBlock
 * @brief A structure to hold the metadata of the file system.
 */
typedef struct superBlock {
    int block_size; /**< The size of each block. */
    int bn;         /**< The number of blocks in the file system. */
    int ffbn;       /**< The block number of the first free block. */
    int rfbn;       /**< The block number of the root folder. */
} superBlock;

/**
 * @struct myDescriptor
 * @brief A structure to hold the metadata of a file or folder.
 */
typedef struct myDescriptor {
    char byte_type[2]; /**< "1" for a file, "2" for a folder. */
    char name[SIZE];   /**< The name of the file or folder. */
    int bn;            /**< The block number of the first block of the file or folder. */
    int size;          /**< The size of the file or folder in bytes. */
} myDescriptor;

/**
 * @brief Creates a new file system.
 * @param filename The name of the file to use for the file system.
 * @param bno The number of blocks in the file system.
 * @param block_size The size of each block.
 * @return 0 on success, -1 on failure.
 */
int createFileSystem(char *filename, int bno, int block_size) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Error creating filesystem");
        return -1;
    }
    superBlock sb;
    sb.block_size = block_size;
    sb.bn = bno;
    sb.ffbn = 1;
    sb.rfbn = 1;
    if (write(fd, &sb, sizeof(superBlock)) != sizeof(superBlock)) {
        perror("Error writing superblock");
        close(fd);
        return -1;
    }
    char *buffer = (char *)malloc(block_size);
    if (buffer == NULL) {
        perror("Error allocating buffer");
        close(fd);
        return -1;
    }
    memset(buffer, 0, block_size);
    for (int i = 0; i < bno; i++) {
        if (write(fd, buffer, block_size) != block_size) {
            perror("Error writing blocks");
            free(buffer);
            close(fd);
            return -1;
        }
    }
    free(buffer);
    close(fd);
    return 0;
}

/**
 * @brief Mounts an existing file system.
 * @param filename The name of the file system to mount.
 * @return 0 on success, -1 on failure.
 */
int mountFileSystem(char *filename) {
    int fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("Error mounting filesystem");
        return -1;
    }
    superBlock sb;
    if (read(fd, &sb, sizeof(superBlock)) != sizeof(superBlock)) {
        perror("Error reading superblock");
        close(fd);
        return -1;
    }
    printf("Filesystem mounted successfully\n");
    printf("Block size: %d\n", sb.block_size);
    printf("Number of blocks: %d\n", sb.bn);
    printf("First free block number: %d\n", sb.ffbn);
    printf("Root folder block number: %d\n", sb.rfbn);
    close(fd);
    return 0;
}

/**
 * @brief Creates a new file in the file system.
 * @param filename The name of the file system.
 * @param file_name The name of the file to create.
 * @return 0 on success, -1 on failure.
 */
int createFile(char *filename, char *file_name) {
    int fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("Error opening filesystem");
        return -1;
    }
    superBlock sb;
    if (read(fd, &sb, sizeof(superBlock)) != sizeof(superBlock)) {
        perror("Error reading superblock");
        close(fd);
        return -1;
    }
    myDescriptor md;
    strcpy(md.byte_type, "1");
    strcpy(md.name, file_name);
    md.bn = sb.ffbn;
    md.size = 0;
    // This is incorrect. The descriptor should be written to the root folder's data block, not to the start of the file system.
    if (write(fd, &md, sizeof(myDescriptor)) != sizeof(myDescriptor)) {
        perror("Error writing file descriptor");
        close(fd);
        return -1;
    }
    sb.ffbn++;
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Error seeking to superblock");
        close(fd);
        return -1;
    }
    if (write(fd, &sb, sizeof(superBlock)) != sizeof(superBlock)) {
        perror("Error writing superblock");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

/**
 * @brief Creates a new folder in the file system.
 * @param filename The name of the file system.
 * @param folder_name The name of the folder to create.
 * @return 0 on success, -1 on failure.
 */
int createFolder(char *filename, char *folder_name) {
    int fd = open(filename, O_RDWR);

    if (fd < 0) {
        perror("Error opening filesystem");
        return -1;
    }

    superBlock sb;
    if (read(fd, &sb, sizeof(superBlock)) != sizeof(superBlock)) {
        perror("Error reading superblock");
        close(fd);
        return -1;
    }

    myDescriptor md;
    strcpy(md.byte_type, "2");
    strcpy(md.name, folder_name);
    md.bn = sb.ffbn;
    md.size = 0;
    // This is incorrect. The descriptor should be written to the root folder's data block.
    if (write(fd, &md, sizeof(myDescriptor)) != sizeof(myDescriptor)) {
        perror("Error writing file descriptor");
        close(fd);
        return -1;
    }
    sb.ffbn++;
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Error seeking to superblock");
        close(fd);
        return -1;
    }

    if (write(fd, &sb, sizeof(superBlock)) != sizeof(superBlock)) {
        perror("Error writing superblock");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

/**
 * @brief A wrapper function for createFileSystem().
 */
void mymkfs(char *filename, int bno, int block_size) {
    if (createFileSystem(filename, bno, block_size) == 0) {
        printf("Filesystem created successfully\n");
    } else {
        printf("Error creating filesystem\n");
    }
}

/**
 * @brief A wrapper function for mountFileSystem().
 */
void mymount(char *filename) {
    if (mountFileSystem(filename) == 0) {
        printf("Filesystem mounted successfully\n");
    } else {
        printf("Error mounting filesystem\n");
    }
}

/**
 * @brief A wrapper function for createFile().
 */
void mycreatefile(char *filename, char *file_name) {
    if (createFile(filename, file_name) == 0) {
        printf("File created successfully\n");
    } else {
        printf("Error creating file\n");
    }
}

/**
 * @brief A wrapper function for createFolder().
 */
void mycreatefolder(char *filename, char *folder_name) {
    if (createFolder(filename, folder_name) == 0) {
        printf("Folder created successfully\n");
    } else {
        printf("Error creating folder\n");
    }
}

/**
 * @brief Copies a file from the host file system to the custom file system.
 * @note This function is incomplete.
 */
void mycopyto(char *filename, char *linux_file) {
    int fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("Error opening filesystem");
        return;
    }
}

int main(int argc, char *argv[]) {
    // Example usage:
    // mymkfs dd1 100 4096
    // mycreatefile dd1 myfile.txt
    // mycreatefolder dd1 myfolder
    return 0;
}



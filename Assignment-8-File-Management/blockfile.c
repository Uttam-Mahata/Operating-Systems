/**
 * @file blockfile.c
 * @brief A simple block-based file system implementation.
 * This program demonstrates how to manage a file as a collection of fixed-size blocks,
 * with a bitmap to keep track of used and free blocks.
 * @note The implementation has some issues. The bitmap is a char array instead of a bit array,
 * which is inefficient. The file size calculation is also incorrect.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 4096

/**
 * @struct BlockRecord
 * @brief A structure to hold the metadata of the file system.
 */
typedef struct blockrec {
    int n;          /**< Number of blocks in the file. */
    int size;       /**< Size of each block. */
    int ubn;        /**< Number of used blocks. */
    int fbn;        /**< Number of free blocks. */
    char ub[256];   /**< Used block bitmap. '1' for used, '0' for free. */
} BlockRecord;

/**
 * @brief Initializes a file with a given number of blocks of a given size.
 * @param fname The name of the file to initialize.
 * @param bsize The size of each block.
 * @param bno The number of blocks.
 * @return 0 on success, -1 on failure.
 */
int init_File_dd(const char *fname, int bsize, int bno) {
    int fd;
    BlockRecord br;
    int i;
    printf("Creating file %s\n", fname);

    fd = open(fname, O_CREAT | O_WRONLY, 0700);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    br.n = bno;
    br.size = bsize;
    br.ubn = 0;
    br.fbn = bno;
    memset(br.ub, '0', sizeof(br.ub));
    printf("Block size: %d, Number of blocks: %d\n", bsize, bno);
    write(fd, (void *)&br, sizeof(BlockRecord));
    for (i = 0; i < bno; i++) {
        char *block = malloc(bsize);
        memset(block, '0', bsize);
        write(fd, block, bsize);
        free(block);
    }
    close(fd);
    return 0;
}

/**
 * @brief Gets the first free block in the file.
 * @param fname The name of the file.
 * @return The block number of the first free block, or -1 if no free blocks are available.
 */
int get_freeblock(const char *fname) {
    int fd;
    BlockRecord br;
    int i;

    fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    read(fd, (void *)&br, sizeof(BlockRecord));
    for (i = 0; i < br.n; i++) {
        if (br.ub[i] == '0') {
            br.ub[i] = '1';
            br.ubn++;
            br.fbn--;
            lseek(fd, 0, SEEK_SET);
            write(fd, (void *)&br, sizeof(BlockRecord));
            close(fd);
            printf("Free block number: %d\n", i);
            return i;
        }
    }

    close(fd);
    return -1;
}

/**
 * @brief Frees a block in the file.
 * @param fname The name of the file.
 * @param bno The block number to free.
 * @return 1 on success, 0 on failure.
 */
int free_block(const char *fname, int bno) {
    int fd;
    BlockRecord br;

    fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 0;
    }

    read(fd, (void *)&br, sizeof(BlockRecord));
    if (br.ub[bno] == '1') {
        br.ub[bno] = '0';
        br.ubn--;
        br.fbn++;
        lseek(fd, 0, SEEK_SET);
        write(fd, (void *)&br, sizeof(BlockRecord));
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}

/**
 * @brief Checks the integrity of the file system.
 * @param fname The name of the file.
 * @return 0 if the file system is consistent, 1 otherwise.
 */
int check_fs(const char *fname) {
    int fd;
    BlockRecord br;

    fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    read(fd, (void *)&br, sizeof(BlockRecord));
    if (br.ubn + br.fbn != br.n) {
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}

/**
 * @brief The main function. It initializes a file system, gets a free block, frees a block, and checks the file system integrity.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char *argv[]) {
    int bsize, bno;
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <filename> <block size> <number of blocks>\n", argv[0]);
        exit(1);
    }
    bsize = atoi(argv[2]);
    bno = atoi(argv[3]);
    init_File_dd(argv[1], bsize, bno);
    get_freeblock(argv[1]);
    free_block(argv[1], 0);
    check_fs(argv[1]);
    return 0;
}


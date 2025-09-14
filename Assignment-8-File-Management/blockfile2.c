/**
 * @file blockfile2.c
 * @brief A more correct implementation of a simple block-based file system.
 * This program demonstrates how to manage a file as a collection of fixed-size blocks,
 * with a bitmap to keep track of used and free blocks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/**
 * @struct file_metadata
 * @brief A structure to hold the metadata of the file system.
 */
typedef struct {
    int n;      // number of blocks
    int s;      // size of each block
    int ubn;    // number of used blocks
    int fbn;    // number of free blocks
    unsigned char ub[]; // bitmap for block status (1 bit per block)
} file_metadata;

/**
 * @brief Calculates the size of the metadata structure.
 * @param n The number of blocks.
 * @return The size of the metadata structure in bytes.
 */
int get_metadata_size(int n) {
    int bitmap_bytes = (n + 7) / 8;
    return sizeof(file_metadata) + bitmap_bytes;
}

/**
 * @brief Reads the metadata from the file.
 * @param fd The file descriptor.
 * @param n The number of blocks.
 * @return A pointer to the metadata structure, or NULL on failure.
 */
file_metadata* read_metadata(int fd, int n) {
    int meta_size = get_metadata_size(n);
    file_metadata* metadata = (file_metadata*)malloc(meta_size);
    
    if (!metadata) {
        perror("Memory allocation failed");
        return NULL;
    }
    
    lseek(fd, 0, SEEK_SET);
    if (read(fd, metadata, meta_size) != meta_size) {
        perror("Failed to read metadata");
        free(metadata);
        return NULL;
    }
    
    return metadata;
}

/**
 * @brief Writes the metadata to the file.
 * @param fd The file descriptor.
 * @param metadata A pointer to the metadata structure.
 * @param n The number of blocks.
 * @return 0 on success, -1 on failure.
 */
int write_metadata(int fd, file_metadata* metadata, int n) {
    int meta_size = get_metadata_size(n);
    
    lseek(fd, 0, SEEK_SET);
    if (write(fd, metadata, meta_size) != meta_size) {
        perror("Failed to write metadata");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Checks if a block is free.
 * @param metadata A pointer to the metadata structure.
 * @param block_num The block number to check.
 * @return 1 if the block is free, 0 otherwise.
 */
int is_block_free(file_metadata* metadata, int block_num) {
    int byte_idx = block_num / 8;
    int bit_idx = block_num % 8;
    return !(metadata->ub[byte_idx] & (1 << bit_idx));
}

/**
 * @brief Sets a block as used in the bitmap.
 * @param metadata A pointer to the metadata structure.
 * @param block_num The block number to set as used.
 */
void set_block_used(file_metadata* metadata, int block_num) {
    int byte_idx = block_num / 8;
    int bit_idx = block_num % 8;
    metadata->ub[byte_idx] |= (1 << bit_idx);
    metadata->ubn++;
    metadata->fbn--;
}

/**
 * @brief Sets a block as free in the bitmap.
 * @param metadata A pointer to the metadata structure.
 * @param block_num The block number to set as free.
 */
void set_block_free(file_metadata* metadata, int block_num) {
    int byte_idx = block_num / 8;
    int bit_idx = block_num % 8;
    metadata->ub[byte_idx] &= ~(1 << bit_idx);
    metadata->ubn--;
    metadata->fbn++;
}

/**
 * @brief Initializes a file with a given number of blocks of a given size.
 * @param fname The name of the file to initialize.
 * @param bsize The size of each block.
 * @param bno The number of blocks.
 * @return 0 on success, -1 on failure.
 */
int init_file_dd(const char *fname, int bsize, int bno) {
    if (bno <= 0 || bsize <= 0) {
        fprintf(stderr, "Invalid block size or block number\n");
        return -1;
    }
    
    int meta_size = get_metadata_size(bno);
    off_t total_size = meta_size + ((off_t)bno * bsize);
    
    int fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Failed to create file");
        return -1;
    }
    
    file_metadata* metadata = (file_metadata*)calloc(1, meta_size);
    if (!metadata) {
        perror("Memory allocation failed");
        close(fd);
        return -1;
    }
    
    metadata->n = bno;
    metadata->s = bsize;
    metadata->ubn = 0;
    metadata->fbn = bno;
    
    if (write_metadata(fd, metadata, bno) != 0) {
        free(metadata);
        close(fd);
        return -1;
    }
    
    if (ftruncate(fd, total_size) == -1) {
        perror("Failed to set file size");
        free(metadata);
        close(fd);
        return -1;
    }
    
    free(metadata);
    close(fd);
    return 0;
}

/**
 * @brief Gets the first free block in the file.
 * @param fname The name of the file.
 * @return The block number of the first free block, or -1 if no free blocks are available.
 */
int get_freeblock(const char *fname) {
    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("Failed to open file");
        return -1;
    }
    
    int n;
    if (read(fd, &n, sizeof(int)) != sizeof(int)) {
        perror("Failed to read block count");
        close(fd);
        return -1;
    }
    
    lseek(fd, 0, SEEK_SET);
    file_metadata* metadata = read_metadata(fd, n);
    if (!metadata) {
        close(fd);
        return -1;
    }
    
    int free_block_num = -1;
    for (int i = 0; i < metadata->n; i++) {
        if (is_block_free(metadata, i)) {
            free_block_num = i;
            set_block_used(metadata, i);
            break;
        }
    }
    
    if (free_block_num == -1) {
        fprintf(stderr, "No free blocks available\n");
        free(metadata);
        close(fd);
        return -1;
    }
    
    if (write_metadata(fd, metadata, n) != 0) {
        free(metadata);
        close(fd);
        return -1;
    }
    
    free(metadata);
    close(fd);
    return free_block_num;
}

/**
 * @brief Frees a block in the file.
 * @param fname The name of the file.
 * @param bno The block number to free.
 * @return 1 on success, 0 if the block is already free, -1 on failure.
 */
int free_block(const char *fname, int bno) {
    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("Failed to open file");
        return -1;
    }
    
    int n;
    if (read(fd, &n, sizeof(int)) != sizeof(int)) {
        perror("Failed to read block count");
        close(fd);
        return -1;
    }
    
    if (bno < 0 || bno >= n) {
        fprintf(stderr, "Invalid block number\n");
        close(fd);
        return -1;
    }
    
    lseek(fd, 0, SEEK_SET);
    file_metadata* metadata = read_metadata(fd, n);
    if (!metadata) {
        close(fd);
        return -1;
    }
    
    if (is_block_free(metadata, bno)) {
        fprintf(stderr, "Block %d is already free\n", bno);
        free(metadata);
        close(fd);
        return 0; 
    }
    
    set_block_free(metadata, bno);
    
    if (write_metadata(fd, metadata, n) != 0) {
        free(metadata);
        close(fd);
        return -1;
    }
    
    free(metadata);
    close(fd);
    return 1;
}

/**
 * @brief Checks the integrity of the file system.
 * @param fname The name of the file.
 * @return 0 if the file system is consistent, -1 otherwise.
 */
int check_fs(const char *fname) {
    int fd = open(fname, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        return -1;
    }
    
    int n;
    if (read(fd, &n, sizeof(int)) != sizeof(int)) {
        perror("Failed to read block count");
        close(fd);
        return -1;
    }
    
    lseek(fd, 0, SEEK_SET);
    file_metadata* metadata = read_metadata(fd, n);
    if (!metadata) {
        close(fd);
        return -1;
    }
    
    int counted_used = 0;
    int counted_free = 0;
    
    for (int i = 0; i < metadata->n; i++) {
        if (is_block_free(metadata, i)) {
            counted_free++;
        } else {
            counted_used++;
        }
    }
    
    if (counted_used != metadata->ubn || counted_free != metadata->fbn) {
        fprintf(stderr, "Inconsistency detected: \n");
        fprintf(stderr, "Metadata: ubn=%d, fbn=%d\n", metadata->ubn, metadata->fbn);
        fprintf(stderr, "Counted: used=%d, free=%d\n", counted_used, counted_free);
        free(metadata);
        close(fd);
        return -1;
    }
    
    free(metadata);
    close(fd);
    return 0;
}

/**
 * @brief Demonstrates the file system functions.
 * @param fname The name of the file to use for the demonstration.
 */
void demonstrate_functions(const char *fname) {
    printf("Initializing file %s with 2048 blocks of 4096 bytes each...\n", fname);
    if (init_file_dd(fname, 4096, 2048) != 0) {
        fprintf(stderr, "Failed to initialize file\n");
        return;
    }
    printf("File initialized successfully.\n");
    
    printf("\nChecking file system integrity...\n");
    if (check_fs(fname) == 0) {
        printf("File system integrity check passed.\n");
    } else {
        printf("File system integrity check failed.\n");
        return;
    }
    
    printf("\nAllocating blocks...\n");
    int blocks[5];
    for (int i = 0; i < 5; i++) {
        blocks[i] = get_freeblock(fname);
        if (blocks[i] == -1) {
            printf("Failed to allocate block %d\n", i);
            return;
        }
        printf("Allocated block %d\n", blocks[i]);
    }
    
    printf("\nFreeing blocks...\n");
    for (int i = 0; i < 3; i++) {
        printf("Freeing block %d...\n", blocks[i]);
        if (free_block(fname, blocks[i]) <= 0) {
            printf("Failed to free block %d\n", blocks[i]);
            return;
        }
        printf("Block %d freed successfully.\n", blocks[i]);
    }
    
    printf("\nChecking file system integrity after operations...\n");
    if (check_fs(fname) == 0) {
        printf("File system integrity check passed.\n");
    } else {
        printf("File system integrity check failed.\n");
    }
}

/**
 * @brief The main function. It runs the demonstration.
 * @return 0 on success.
 */
int main() {
    const char* filename = "dd1";
    demonstrate_functions(filename);
    return 0;
}
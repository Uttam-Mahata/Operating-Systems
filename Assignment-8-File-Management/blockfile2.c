#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

// Function prototypes
int init_file_dd(const char *fname, int bsize, int bno);
int get_freeblock(const char *fname);
int free_block(const char *fname, int bno);
int check_fs(const char *fname);
void demonstrate_functions(const char *fname);

// Structure to hold file metadata
typedef struct {
    int n;      // number of blocks
    int s;      // size of each block
    int ubn;    // number of used blocks
    int fbn;    // number of free blocks
    unsigned char ub[]; // bitmap for block status (1 bit per block)
} file_metadata;

int get_metadata_size(int n) {
    // Calculate how many bytes we need for the bitmap
    // Each byte can store 8 blocks' status
    int bitmap_bytes = (n + 7) / 8;
    return sizeof(file_metadata) + bitmap_bytes;
}

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

int write_metadata(int fd, file_metadata* metadata, int n) {
    int meta_size = get_metadata_size(n);
    
    lseek(fd, 0, SEEK_SET);
    if (write(fd, metadata, meta_size) != meta_size) {
        perror("Failed to write metadata");
        return -1;
    }
    
    return 0;
}


// check if a block is free
int is_block_free(file_metadata* metadata, int block_num) {
    int byte_idx = block_num / 8;
    int bit_idx = block_num % 8;
    
    // A 0 bit indicates the block is free
    return !(metadata->ub[byte_idx] & (1 << bit_idx));
}

void set_block_used(file_metadata* metadata, int block_num) {
    int byte_idx = block_num / 8;
    int bit_idx = block_num % 8;
    
    // Set the bit to 1 (used)
    metadata->ub[byte_idx] |= (1 << bit_idx);
    metadata->ubn++;
    metadata->fbn--;
}

void set_block_free(file_metadata* metadata, int block_num) {
    int byte_idx = block_num / 8;
    int bit_idx = block_num % 8;
    
    // Set the bit to 0 (free)
    metadata->ub[byte_idx] &= ~(1 << bit_idx);
    metadata->ubn--;
    metadata->fbn++;
}

int init_file_dd(const char *fname, int bsize, int bno) {
    if (bno <= 0 || bsize <= 0) {
        fprintf(stderr, "Invalid block size or block number\n");
        return -1;
    }
    
    int meta_size = get_metadata_size(bno);
    off_t total_size = meta_size + ((off_t)bno * bsize);
    
    // Create and open the file
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
    
    // Extend the file to the required size
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

int get_freeblock(const char *fname) {
    // Open the file
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
    
    // Read the complete metadata
    lseek(fd, 0, SEEK_SET);
    file_metadata* metadata = read_metadata(fd, n);
    if (!metadata) {
        close(fd);
        return -1;
    }
    
    // Look for a free block
    int free_block = -1;
    for (int i = 0; i < metadata->n; i++) {
        if (is_block_free(metadata, i)) {
            free_block = i;
            // Mark it as used
            set_block_used(metadata, i);
            break;
        }
    }
    
    if (free_block == -1) {
        fprintf(stderr, "No free blocks available\n");
        free(metadata);
        close(fd);
        return -1;
    }
    
    // Update metadata in the file
    if (write_metadata(fd, metadata, n) != 0) {
        free(metadata);
        close(fd);
        return -1;
    }
    
    // Clean up and return the found free block
    free(metadata);
    close(fd);
    return free_block;
}

// Function to mark a block as free
int free_block(const char *fname, int bno) {
    // Open the file
    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("Failed to open file");
        return -1;
    }
    
    // Read the number of blocks from the file
    int n;
    if (read(fd, &n, sizeof(int)) != sizeof(int)) {
        perror("Failed to read block count");
        close(fd);
        return -1;
    }
    
    // Check if the block number is valid
    if (bno < 0 || bno >= n) {
        fprintf(stderr, "Invalid block number\n");
        close(fd);
        return -1;
    }
    
    // Read the complete metadata
    lseek(fd, 0, SEEK_SET);
    file_metadata* metadata = read_metadata(fd, n);
    if (!metadata) {
        close(fd);
        return -1;
    }
    
    // Check if the block is already free
    if (is_block_free(metadata, bno)) {
        fprintf(stderr, "Block %d is already free\n", bno);
        free(metadata);
        close(fd);
        return 0; 
    }
    
    // Mark the block as free
    set_block_free(metadata, bno);
    
    // Update metadata in the file
    if (write_metadata(fd, metadata, n) != 0) {
        free(metadata);
        close(fd);
        return -1;
    }
    
    // Clean up and return success
    free(metadata);
    close(fd);
    return 1;
}

// Function to check the integrity of the file
int check_fs(const char *fname) {
    // Open the file
    int fd = open(fname, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        return -1;
    }
    
    // Read the number of blocks from the file
    int n;
    if (read(fd, &n, sizeof(int)) != sizeof(int)) {
        perror("Failed to read block count");
        close(fd);
        return -1;
    }
    
    // Read the complete metadata
    lseek(fd, 0, SEEK_SET);
    file_metadata* metadata = read_metadata(fd, n);
    if (!metadata) {
        close(fd);
        return -1;
    }
    
    // Count used and free blocks from the bitmap
    int counted_used = 0;
    int counted_free = 0;
    
    for (int i = 0; i < metadata->n; i++) {
        if (is_block_free(metadata, i)) {
            counted_free++;
        } else {
            counted_used++;
        }
    }
    
    // Check for inconsistencies
    if (counted_used != metadata->ubn || counted_free != metadata->fbn) {
        fprintf(stderr, "Inconsistency detected: \n");
        fprintf(stderr, "Metadata: ubn=%d, fbn=%d\n", metadata->ubn, metadata->fbn);
        fprintf(stderr, "Counted: used=%d, free=%d\n", counted_used, counted_free);
        free(metadata);
        close(fd);
        return -1;
    }
    
    // Clean up and return success
    free(metadata);
    close(fd);
    return 0;
}

void demonstrate_functions(const char *fname) {
    // Initialize a file with 10 blocks of 512 bytes each
    printf("Initializing file %s with 2048 blocks of 4096 bytes each...\n", fname);
    if (init_file_dd(fname, 4096, 2048) != 0) {
        fprintf(stderr, "Failed to initialize file\n");
        return;
    }
    printf("File initialized successfully.\n");
    
    // Check file system integrity
    printf("\nChecking file system integrity...\n");
    if (check_fs(fname) == 0) {
        printf("File system integrity check passed.\n");
    } else {
        printf("File system integrity check failed.\n");
        return;
    }
    
    // Get some free blocks
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
    
    // Free some blocks
    printf("\nFreeing blocks...\n");
    for (int i = 0; i < 3; i++) {
        printf("Freeing block %d...\n", blocks[i]);
        if (free_block(fname, blocks[i]) <= 0) {
            printf("Failed to free block %d\n", blocks[i]);
            return;
        }
        printf("Block %d freed successfully.\n", blocks[i]);
    }
    
    // Check file system integrity again
    printf("\nChecking file system integrity after operations...\n");
    if (check_fs(fname) == 0) {
        printf("File system integrity check passed.\n");
    } else {
        printf("File system integrity check failed.\n");
    }
}

// Main function
int main() {
    const char* filename = "dd1";
    demonstrate_functions(filename);
    return 0;
}
/* 
 * blockfile.c - Implementation of an interface to maintain files as a collection of blocks
 * Built on top of random access file functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Block size definitions */
#define BLOCK_SIZE 1024        /* Size of each block's data portion in bytes */
#define MAX_BLOCKS 1000        /* Maximum number of blocks a file can have */



/* Block structure definition */
typedef struct {
    int block_id;              /* Unique identifier for the block */
    int used_size;             /* Actual data size used in the block (can be <= BLOCK_SIZE) */
    char data[BLOCK_SIZE];     /* The actual data stored in the block */
} Block;

/* Block File Control structure - maintains metadata about our block file */
typedef struct {
    int fd;                    /* File descriptor of the opened file */
    int total_blocks;          /* Total number of blocks in the file */
    int block_size;            /* Size of each block in bytes (including metadata) */
    char filename[256];        /* Name of the block file */
} BlockFile;

/* Function prototypes */
BlockFile* createBlockFile(const char *filename);
BlockFile* openBlockFile(const char *filename);
int closeBlockFile(BlockFile *bf);
int writeBlock(BlockFile *bf, Block *block, int block_num);
int readBlock(BlockFile *bf, Block *block, int block_num);
int addBlock(BlockFile *bf, Block *block);
int deleteBlock(BlockFile *bf, int block_num);
void displayBlock(Block *block);
void initBlock(Block *block, int id);

/* Create a new block file with the given name */
BlockFile* createBlockFile(const char *filename) {
    int fd;
    BlockFile *bf = (BlockFile*)malloc(sizeof(BlockFile));
    if (!bf) {
        perror("Memory allocation failed");
        return NULL;
    }

    /* Create the file with read/write permissions */
    fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Failed to create block file");
        free(bf);
        return NULL;
    }
    
    /* Initialize the BlockFile control structure */
    bf->fd = fd;
    bf->total_blocks = 0;
    bf->block_size = sizeof(Block);
    strncpy(bf->filename, filename, 255);
    bf->filename[255] = '\0';
    
    /* Write the initial block count to the file */
    if (write(fd, &bf->total_blocks, sizeof(int)) != sizeof(int)) {
        perror("Failed to write block file header");
        close(fd);
        free(bf);
        return NULL;
    }
    
    return bf;
}

/* Open an existing block file */
BlockFile* openBlockFile(const char *filename) {
    int fd;
    BlockFile *bf = (BlockFile*)malloc(sizeof(BlockFile));
    if (!bf) {
        perror("Memory allocation failed");
        return NULL;
    }

    fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("Failed to open block file");
        free(bf);
        return NULL;
    }
    
    /* Read the block count from the file header */
    if (read(fd, &bf->total_blocks, sizeof(int)) != sizeof(int)) {
        perror("Failed to read block file header");
        close(fd);
        free(bf);
        return NULL;
    }
    
    /* Initialize the rest of the BlockFile structure */
    bf->fd = fd;
    bf->block_size = sizeof(Block);
    strncpy(bf->filename, filename, 255);
    bf->filename[255] = '\0';
    
    return bf;
}

/* Close a block file and free resources */
int closeBlockFile(BlockFile *bf) {
    int result = 0;
    if (!bf) {
        fprintf(stderr, "Invalid BlockFile pointer\n");
        return -1;
    }
    
    /* Seek to the beginning of the file to update the block count */
    if (lseek(bf->fd, 0, SEEK_SET) == -1) {
        perror("Failed to seek to file header");
        result = -1;
    } else {
        /* Write the updated block count back to the file */
        if (write(bf->fd, &bf->total_blocks, sizeof(int)) != sizeof(int)) {
            perror("Failed to update block file header");
            result = -1;
        }
    }
    
    /* Close the file and free memory */
    close(bf->fd);
    free(bf);
    
    return result;
}

/* Write a block to a specific position in the block file */
int writeBlock(BlockFile *bf, Block *block, int block_num) {
    off_t offset;
    ssize_t bytes_written;
    
    if (!bf || !block) {
        fprintf(stderr, "Invalid BlockFile or Block pointer\n");
        return -1;
    }
    
    if (block_num < 0 || block_num >= MAX_BLOCKS) {
        fprintf(stderr, "Block number out of range\n");
        return -1;
    }
    
    /* Calculate position: header size + (block_num * block_size) */
    offset = sizeof(int) + (block_num * bf->block_size);
    
    /* Seek to the correct position */
    if (lseek(bf->fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek in block file");
        return -1;
    }
    
    /* Write the block */
    bytes_written = write(bf->fd, block, bf->block_size);
    if (bytes_written != bf->block_size) {
        perror("Failed to write block");
        return -1;
    }
    
    /* Update total_blocks if necessary */
    if (block_num >= bf->total_blocks) {
        bf->total_blocks = block_num + 1;
    }
    
    return 0;
}

/* Read a block from a specific position in the block file */
int readBlock(BlockFile *bf, Block *block, int block_num) {
    off_t offset;
    ssize_t bytes_read;
    
    if (!bf || !block) {
        fprintf(stderr, "Invalid BlockFile or Block pointer\n");
        return -1;
    }
    
    if (block_num < 0 || block_num >= bf->total_blocks) {
        fprintf(stderr, "Block number out of range\n");
        return -1;
    }
    
    /* Calculate position: header size + (block_num * block_size) */
    offset = sizeof(int) + (block_num * bf->block_size);
    
    /* Seek to the correct position */
    if (lseek(bf->fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek in block file");
        return -1;
    }
    
    /* Read the block */
    bytes_read = read(bf->fd, block, bf->block_size);
    if (bytes_read != bf->block_size) {
        perror("Failed to read block");
        return -1;
    }
    
    return 0;
}

/* Add a new block to the end of the block file */
int addBlock(BlockFile *bf, Block *block) {
    if (!bf || !block) {
        fprintf(stderr, "Invalid BlockFile or Block pointer\n");
        return -1;
    }
    
    if (bf->total_blocks >= MAX_BLOCKS) {
        fprintf(stderr, "Maximum number of blocks reached\n");
        return -1;
    }
    
    /* Write the block at the end of the file */
    return writeBlock(bf, block, bf->total_blocks);
}

/* Delete a block by marking it as unused */
int deleteBlock(BlockFile *bf, int block_num) {
    Block empty_block;
    
    if (!bf) {
        fprintf(stderr, "Invalid BlockFile pointer\n");
        return -1;
    }
    
    if (block_num < 0 || block_num >= bf->total_blocks) {
        fprintf(stderr, "Block number out of range\n");
        return -1;
    }
    
    /* Initialize an empty block with -1 as block_id (indicating deleted) */
    memset(&empty_block, 0, sizeof(Block));
    empty_block.block_id = -1;
    empty_block.used_size = 0;
    
    /* Write the empty block to the position */
    return writeBlock(bf, &empty_block, block_num);
}

/* Display block contents */
void displayBlock(Block *block) {
    if (!block) {
        fprintf(stderr, "Invalid Block pointer\n");
        return;
    }
    
    printf("Block ID: %d\n", block->block_id);
    printf("Used Size: %d bytes\n", block->used_size);
    printf("Data: ");
    
    /* Display only the used portion of the data */
    if (block->used_size > 0) {
        int i;
        /* Show first few bytes as characters if printable, otherwise as hex */
        for (i = 0; i < (block->used_size > 20 ? 20 : block->used_size); i++) {
            if (block->data[i] >= 32 && block->data[i] <= 126) {
                printf("%c", block->data[i]);
            } else {
                printf("\\x%02x", (unsigned char)block->data[i]);
            }
        }
        if (block->used_size > 20) {
            printf("...");
        }
    } else {
        printf("(empty)");
    }
    printf("\n");
}

/* Initialize a block with a given ID */
void initBlock(Block *block, int id) {
    if (!block) {
        fprintf(stderr, "Invalid Block pointer\n");
        return;
    }
    
    memset(block, 0, sizeof(Block));
    block->block_id = id;
    block->used_size = 0;
}

/* Main function for demonstration */
int main() {
    BlockFile *bf;
    Block block;
    char test_data[] = "This is test data for our block file implementation.";
    int i, status;
    
    /* Create a new block file */
    printf("Creating block file...\n");
    bf = createBlockFile("test_blocks.dat");
    if (!bf) {
        fprintf(stderr, "Failed to create block file\n");
        return 1;
    }
    
    /* Add some blocks with test data */
    printf("Adding blocks with test data...\n");
    for (i = 0; i < 5; i++) {
        initBlock(&block, i);
        snprintf(block.data, BLOCK_SIZE, "%s Block %d", test_data, i);
        block.used_size = strlen(block.data) + 1;
        status = addBlock(bf, &block);
        if (status != 0) {
            fprintf(stderr, "Failed to add block %d\n", i);
        }
    }
    
    /* Close the file */
    closeBlockFile(bf);
    
    /* Reopen the file and read blocks */
    printf("\nReopening block file and reading blocks...\n");
    bf = openBlockFile("test_blocks.dat");
    if (!bf) {
        fprintf(stderr, "Failed to open block file\n");
        return 1;
    }
    
    printf("Total blocks: %d\n", bf->total_blocks);
    
    /* Read and display all blocks */
    for (i = 0; i < bf->total_blocks; i++) {
        printf("\nReading block %d:\n", i);
        status = readBlock(bf, &block, i);
        if (status == 0) {
            displayBlock(&block);
        } else {
            fprintf(stderr, "Failed to read block %d\n", i);
        }
    }
    
    /* Delete a block */
    printf("\nDeleting block 2...\n");
    status = deleteBlock(bf, 2);
    if (status != 0) {
        fprintf(stderr, "Failed to delete block 2\n");
    }
    
    /* Read the deleted block */
    printf("Reading deleted block 2:\n");
    status = readBlock(bf, &block, 2);
    if (status == 0) {
        displayBlock(&block);
    }
    
    /* Close the file */
    closeBlockFile(bf);
    
    printf("\nBlock file operations completed.\n");
    
    return 0;
}

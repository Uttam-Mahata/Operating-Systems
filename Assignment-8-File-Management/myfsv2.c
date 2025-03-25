/* Filename: myfsv2.c */

/* Extended version of myfs with support for:
   1. Files and folders spanning multiple blocks (using chained blocks)
   2. Hierarchical directory structure
   3. Operations: mymkfs, mycopyTo, mycopyFrom, myrm, myrmdir */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <stdint.h>

#define BS 4096
#define BNO 2048
#define NOFILES 2048
#define FNLEN 12
#define DESCRIPTOR_SIZE 21
#define TYPE_FILE 1
#define TYPE_FOLDER 2
#define ROOT_BLOCK 1

char buf[BS];
char sbuf[8*BS]; // Super block buffer

/* Function prototypes */
int mymkfs(const char *fname, int num_blocks);
int mycopyTo(const char *src, const char *dest, const char *dest_folder);
int mycopyFrom(const char *src, const char *dest);
int myrm(const char *path);
int myrmdir(const char *path);
int myreadSBlocks(int fd, char *sbuf);
int mywriteSBlocks(int fd, char *sbuf);
int myreadBlock(int fd, int bno, char *buf);
int mywriteBlock(int fd, int bno, char *buf);
int findFreeBlock(int fd);
int allocateBlock(int fd, int *block_num);
void freeBlock(int fd, int block_num);
int freeChain(int fd, int first_block);
int findFile(int fd, const char *path, int *parent_block, int *file_offset, int *file_block);
int createFileDescriptor(int fd, const char *filename, char type, int first_block, int size, int parent_block);
char** parseFilePath(const char *path, int *count);
void freePathComponents(char **components, int count);

int main(int argc, char *argv[]) {
    char *basename;
    int flag;

    basename = strrchr(argv[0], '/');
    if (basename != NULL) {
        basename++;
    } else {
        basename = argv[0];
    }

    if (strcmp(basename, "mymkfs") == 0) {
        if(argc != 2) {
            fprintf(stderr, "Usage: %s <linux file>\n", argv[0]);
            exit(1);
        }
        flag = mymkfs(argv[1], BNO + 8); // Create filesystem with default size

        if (flag != 0) {
            fprintf(stderr, "%s Failed!\n", argv[0]);
        }
    } else if (strcmp(basename, "mycopyTo") == 0) {
        if (argc < 3 || argc > 4) {
            fprintf(stderr, "Usage: %s <linux file> <storage file> [destination folder path]\n", argv[0]);
            exit(1);
        }
        
        const char *dest_folder = "/"; // Default to root
        if (argc == 4) {
            dest_folder = argv[3];
        }
        
        flag = mycopyTo(argv[1], argv[2], dest_folder);
        if (flag != 0) {
            fprintf(stderr, "%s Failed!\n", argv[0]);
        }
    } else if (strcmp(basename, "mycopyFrom") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s <myfile path>@<storage file> <linux file>\n", argv[0]);
            exit(1);
        }
        flag = mycopyFrom(argv[1], argv[2]);
        if (flag != 0) {
            fprintf(stderr, "%s Failed!\n", argv[0]);
        }
    } else if (strcmp(basename, "myrm") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Usage: %s <myfile path>@<storage file>\n", argv[0]);
            exit(1);
        }
        flag = myrm(argv[1]);
        if (flag != 0) {
            fprintf(stderr, "%s Failed!\n", argv[0]);
        }
    } else if (strcmp(basename, "myrmdir") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Usage: %s <myfolder path>@<storage file>\n", argv[0]);
            exit(1);
        }
        flag = myrmdir(argv[1]);
        if (flag != 0) {
            fprintf(stderr, "%s Failed!\n", argv[0]);
        }
    } else {
        fprintf(stderr, "%s: Command not found!\n", argv[0]);
    }
    
    return 0;
}

/* Initialize a new filesystem */
int mymkfs(const char *fname, int num_blocks) {
    int fd;
    int i;
    int flag;
    
    fd = open(fname, O_CREAT | O_WRONLY, S_IRWXU);
    if (fd == -1) {
        fprintf(stderr, "%s: ", fname);
        perror("File cannot be opened for writing");
        return (-1);
    }
    
    /* Initialize all blocks to zero */
    memset(buf, 0, BS);
    for (i = 0; i < num_blocks; i++) {
        flag = write(fd, buf, BS);
        if (flag == -1) {
            fprintf(stderr, "%s: ", fname);
            perror("File write failed!");
            return (-1);
        }
    }
    
    /* Initialize superblock with filesystem metadata */
    memset(sbuf, 0, 8*BS);
    *((int *)sbuf) = BS;                  // Block size
    *((int *)(sbuf + 4)) = num_blocks;    // Number of blocks
    *((int *)(sbuf + 8)) = 2;             // First free block (after root)
    *((int *)(sbuf + 12)) = ROOT_BLOCK;   // Root folder block
    
    flag = mywriteSBlocks(fd, sbuf);
    if (flag == -1) {
        fprintf(stderr, "Failed to write superblock!\n");
        close(fd);
        return (-1);
    }
    
    /* Initialize root folder block */
    memset(buf, 0, BS);
    buf[0] = TYPE_FOLDER;                 // Type: folder
    strncpy(buf + 1, "/", FNLEN);         // Name: "/"
    *((int *)(buf + 13)) = ROOT_BLOCK;    // First block is itself
    *((int *)(buf + 17)) = 0;             // Initially empty (size 0)
    
    flag = mywriteBlock(fd, ROOT_BLOCK, buf);
    if (flag == -1) {
        fprintf(stderr, "Failed to initialize root folder!\n");
        close(fd);
        return (-1);
    }
    
    close(fd);
    return 0;
}

/* Copy a file from Linux to our filesystem */
int mycopyTo(const char *src, const char *dest, const char *dest_folder) {
    int fd_src, fd_dest;
    int flag;
    int first_block = -1;
    int current_block = -1;
    int parent_block = -1;
    int parent_offset = -1;
    int dummy = -1;
    int bytes_read, total_bytes = 0;
    struct stat sb;
    
    /* Check source file */
    flag = stat(src, &sb);
    if (flag == -1) {
        fprintf(stderr, "File %s: ", src);
        perror("stat() failed");
        return (-1);
    }
    
    if (strlen(src) > FNLEN) {
        fprintf(stderr, "File %s name is too long (max %d characters)\n", src, FNLEN);
        return (-1);
    }
    
    /* Open source file */
    fd_src = open(src, O_RDONLY);
    if (fd_src == -1) {
        fprintf(stderr, "File %s: ", src);
        perror("Cannot be opened for reading");
        return (-1);
    }
    
    /* Open destination filesystem */
    fd_dest = open(dest, O_RDWR);
    if (fd_dest == -1) {
        fprintf(stderr, "File %s: ", dest);
        perror("Cannot be opened for writing");
        close(fd_src);
        return (-1);
    }
    
    /* Check if the destination folder exists */
    flag = findFile(fd_dest, dest_folder, &dummy, &dummy, &parent_block);
    if (flag == -1 || parent_block == -1) {
        fprintf(stderr, "Destination folder %s not found\n", dest_folder);
        close(fd_src);
        close(fd_dest);
        return (-1);
    }
    
    /* Check if file with same name already exists */
    const char *basename = strrchr(src, '/');
    if (basename == NULL) {
        basename = src;
    } else {
        basename++;
    }
    
    char full_path[256];
    if (strcmp(dest_folder, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "/%s", basename);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", dest_folder, basename);
    }
    
    flag = findFile(fd_dest, full_path, &parent_block, &parent_offset, &dummy);
    if (flag == 0) {
        fprintf(stderr, "File %s already exists in destination\n", full_path);
        close(fd_src);
        close(fd_dest);
        return (-1);
    }
    
    /* Copy file data in blocks */
    do {
        /* Get a free block */
        flag = allocateBlock(fd_dest, &current_block);
        if (flag == -1) {
            fprintf(stderr, "No free blocks available\n");
            if (first_block != -1) {
                freeChain(fd_dest, first_block);
            }
            close(fd_src);
            close(fd_dest);
            return (-1);
        }
        
        if (first_block == -1) {
            first_block = current_block;
        } else {
            /* Link previous block to current block */
            memset(buf, 0, BS);
            flag = myreadBlock(fd_dest, current_block - 1, buf);
            if (flag == -1) {
                freeChain(fd_dest, first_block);
                close(fd_src);
                close(fd_dest);
                return (-1);
            }
            
            /* Set next block pointer (last 4 bytes) */
            *((int *)(buf + BS - 4)) = current_block;
            
            flag = mywriteBlock(fd_dest, current_block - 1, buf);
            if (flag == -1) {
                freeChain(fd_dest, first_block);
                close(fd_src);
                close(fd_dest);
                return (-1);
            }
        }
        
        /* Read data from source file */
        memset(buf, 0, BS);
        bytes_read = read(fd_src, buf, BS - 4); /* Leave 4 bytes for next block pointer */
        if (bytes_read == -1) {
            fprintf(stderr, "File %s: ", src);
            perror("Read error");
            freeChain(fd_dest, first_block);
            close(fd_src);
            close(fd_dest);
            return (-1);
        }
        
        total_bytes += bytes_read;
        
        /* Write data to destination block */
        /* Last 4 bytes will be the next block (0 if last block) */
        *((int *)(buf + BS - 4)) = 0;
        
        flag = mywriteBlock(fd_dest, current_block, buf);
        if (flag == -1) {
            freeChain(fd_dest, first_block);
            close(fd_src);
            close(fd_dest);
            return (-1);
        }
        
    } while (bytes_read == BS - 4);
    
    /* Create file descriptor in parent folder */
    flag = createFileDescriptor(fd_dest, basename, TYPE_FILE, first_block, total_bytes, parent_block);
    if (flag == -1) {
        fprintf(stderr, "Failed to create file descriptor\n");
        freeChain(fd_dest, first_block);
        close(fd_src);
        close(fd_dest);
        return (-1);
    }
    
    close(fd_src);
    close(fd_dest);
    return 0;
}

/* Copy a file from our filesystem to Linux */
int mycopyFrom(const char *src, const char *dest) {
    int fd_src, fd_dest;
    int flag;
    int parent_block, file_offset, file_block;
    int next_block;
    int bytes_to_read, bytes_written;
    int total_bytes = 0;
    
    /* Parse source path: <myfile path>@<storage file> */
    char src_copy[256];
    strncpy(src_copy, src, sizeof(src_copy));
    
    char *storage_file = strchr(src_copy, '@');
    if (storage_file == NULL) {
        fprintf(stderr, "Invalid source format. Use: <myfile path>@<storage file>\n");
        return (-1);
    }
    *storage_file = '\0';
    storage_file++;
    
    char *myfile_path = src_copy;
    
    /* Open source filesystem */
    fd_src = open(storage_file, O_RDONLY);
    if (fd_src == -1) {
        fprintf(stderr, "File %s: ", storage_file);
        perror("Cannot be opened for reading");
        return (-1);
    }
    
    /* Find the file in the filesystem */
    flag = findFile(fd_src, myfile_path, &parent_block, &file_offset, &file_block);
    if (flag == -1 || file_block == -1) {
        fprintf(stderr, "File %s not found in filesystem %s\n", myfile_path, storage_file);
        close(fd_src);
        return (-1);
    }
    
    /* Read file descriptor to get file size */
    memset(buf, 0, BS);
    flag = myreadBlock(fd_src, parent_block, buf);
    if (flag == -1) {
        close(fd_src);
        return (-1);
    }
    
    char file_type = buf[file_offset];
    int file_size = *((int *)(buf + file_offset + 17));
    
    if (file_type != TYPE_FILE) {
        fprintf(stderr, "%s is not a file\n", myfile_path);
        close(fd_src);
        return (-1);
    }
    
    /* Open destination file */
    fd_dest = open(dest, O_CREAT | O_WRONLY, S_IRWXU);
    if (fd_dest == -1) {
        fprintf(stderr, "File %s: ", dest);
        perror("Cannot be opened for writing");
        close(fd_src);
        return (-1);
    }
    
    /* Copy file data from chained blocks */
    next_block = file_block;
    
    while (next_block != 0 && total_bytes < file_size) {
        /* Read block data */
        flag = myreadBlock(fd_src, next_block, buf);
        if (flag == -1) {
            close(fd_src);
            close(fd_dest);
            return (-1);
        }
        
        /* Get next block from the last 4 bytes */
        next_block = *((int *)(buf + BS - 4));
        
        /* Calculate how many bytes to write from this block */
        bytes_to_read = (file_size - total_bytes < BS - 4) ? 
                         file_size - total_bytes : BS - 4;
        
        /* Write data to destination file */
        bytes_written = write(fd_dest, buf, bytes_to_read);
        if (bytes_written == -1) {
            fprintf(stderr, "File %s: ", dest);
            perror("Write error");
            close(fd_src);
            close(fd_dest);
            return (-1);
        }
        
        total_bytes += bytes_written;
    }
    
    close(fd_src);
    close(fd_dest);
    return 0;
}

/* Remove a file from our filesystem */
int myrm(const char *path) {
    int fd;
    int flag;
    int parent_block, file_offset, file_block;
    
    /* Parse path: <myfile path>@<storage file> */
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));
    
    char *storage_file = strchr(path_copy, '@');
    if (storage_file == NULL) {
        fprintf(stderr, "Invalid path format. Use: <myfile path>@<storage file>\n");
        return (-1);
    }
    *storage_file = '\0';
    storage_file++;
    
    char *myfile_path = path_copy;
    
    /* Open filesystem */
    fd = open(storage_file, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s: ", storage_file);
        perror("Cannot be opened");
        return (-1);
    }
    
    /* Find the file in the filesystem */
    flag = findFile(fd, myfile_path, &parent_block, &file_offset, &file_block);
    if (flag == -1 || file_block == -1) {
        fprintf(stderr, "File %s not found in filesystem %s\n", myfile_path, storage_file);
        close(fd);
        return (-1);
    }
    
    /* Read parent folder to check file type */
    memset(buf, 0, BS);
    flag = myreadBlock(fd, parent_block, buf);
    if (flag == -1) {
        close(fd);
        return (-1);
    }
    
    char file_type = buf[file_offset];
    if (file_type != TYPE_FILE) {
        fprintf(stderr, "%s is not a file (use myrmdir for folders)\n", myfile_path);
        close(fd);
        return (-1);
    }
    
    /* Free the file's blocks */
    flag = freeChain(fd, file_block);
    if (flag == -1) {
        close(fd);
        return (-1);
    }
    
    /* Clear the file's descriptor in the parent folder */
    buf[file_offset] = 0; // Mark as deleted
    
    flag = mywriteBlock(fd, parent_block, buf);
    if (flag == -1) {
        close(fd);
        return (-1);
    }
    
    close(fd);
    return 0;
}

/* Remove a folder from our filesystem */
int myrmdir(const char *path) {
    int fd;
    int flag;
    int parent_block, folder_offset, folder_block;
    int i, num_entries;
    
    /* Parse path: <myfolder path>@<storage file> */
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));
    
    char *storage_file = strchr(path_copy, '@');
    if (storage_file == NULL) {
        fprintf(stderr, "Invalid path format. Use: <myfolder path>@<storage file>\n");
        return (-1);
    }
    *storage_file = '\0';
    storage_file++;
    
    char *myfolder_path = path_copy;
    
    /* Check if trying to delete root folder */
    if (strcmp(myfolder_path, "/") == 0) {
        fprintf(stderr, "Cannot delete root folder\n");
        return (-1);
    }
    
    /* Open filesystem */
    fd = open(storage_file, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "File %s: ", storage_file);
        perror("Cannot be opened");
        return (-1);
    }
    
    /* Find the folder in the filesystem */
    flag = findFile(fd, myfolder_path, &parent_block, &folder_offset, &folder_block);
    if (flag == -1 || folder_block == -1) {
        fprintf(stderr, "Folder %s not found in filesystem %s\n", myfolder_path, storage_file);
        close(fd);
        return (-1);
    }
    
    /* Read parent folder to check folder type */
    memset(buf, 0, BS);
    flag = myreadBlock(fd, parent_block, buf);
    if (flag == -1) {
        close(fd);
        return (-1);
    }
    
    char folder_type = buf[folder_offset];
    if (folder_type != TYPE_FOLDER) {
        fprintf(stderr, "%s is not a folder\n", myfolder_path);
        close(fd);
        return (-1);
    }
    
    /* Read folder block to check if it's empty */
    memset(buf, 0, BS);
    flag = myreadBlock(fd, folder_block, buf);
    if (flag == -1) {
        close(fd);
        return (-1);
    }
    
    /* Check if folder is empty by scanning for entries */
    /* Skip first 21 bytes which are the folder's own descriptor */
    for (i = DESCRIPTOR_SIZE; i < BS - 4; i += DESCRIPTOR_SIZE) {
        if (buf[i] != 0) {
            fprintf(stderr, "Cannot delete non-empty folder %s\n", myfolder_path);
            close(fd);
            return (-1);
        }
    }
    
    /* Free the folder's blocks */
    flag = freeChain(fd, folder_block);
    if (flag == -1) {
        close(fd);
        return (-1);
    }
    
    /* Clear the folder's descriptor in the parent folder */
    memset(buf, 0, BS);
    flag = myreadBlock(fd, parent_block, buf);
    if (flag == -1) {
        close(fd);
        return (-1);
    }
    
    buf[folder_offset] = 0; // Mark as deleted
    
    flag = mywriteBlock(fd, parent_block, buf);
    if (flag == -1) {
        close(fd);
        return (-1);
    }
    
    close(fd);
    return 0;
}

/* Helper functions */

/* Read the 8 superblocks */
int myreadSBlocks(int fd, char *sbuf) {
    int i;
    int flag;
    flag = lseek(fd, 0, SEEK_SET);
    if (flag == -1) {
        perror("lseek() at myreadSBlocks() fails");
        return (flag);
    }
    flag = 0;
    for (i = 0; i < 8 && flag != -1; i++) {
        flag = myreadBlock(fd, i, &(sbuf[i*BS]));
    }
    return (flag);
}

/* Write the 8 superblocks */
int mywriteSBlocks(int fd, char *sbuf) {
    int i;
    int flag;
    flag = lseek(fd, 0, SEEK_SET);
    if (flag == -1) {
        perror("lseek() at mywriteSBlocks() fails");
        return (-1);
    }
    flag = 0;
    for (i = 0; i < 8 && flag != -1; i++) {
        flag = mywriteBlock(fd, i, &(sbuf[i*BS]));
    }
    return (flag);
}

/* Read a block from the filesystem */
int myreadBlock(int fd, int bno, char *buf) {
    int flag;
    flag = lseek(fd, bno * BS, SEEK_SET);
    if (flag == -1) {
        perror("lseek() at myreadBlock() fails");
        return (flag);
    }
    flag = read(fd, buf, BS);
    if (flag == -1) {
        perror("read() at myreadBlock() fails");
        return (-1);
    }
    return (0);
}

/* Write a block to the filesystem */
int mywriteBlock(int fd, int bno, char *buf) {
    int flag;
    flag = lseek(fd, bno * BS, SEEK_SET);
    if (flag == -1) {
        perror("lseek() at mywriteBlock() fails");
        return (flag);
    }
    flag = write(fd, buf, BS);
    if (flag == -1) {
        perror("write() at mywriteBlock() fails");
        return (-1);
    }
    return (0);
}

/* Find a free block in the filesystem */
int findFreeBlock(int fd) {
    int free_block;
    
    /* Read superblock to get first free block */
    memset(sbuf, 0, 8*BS);
    if (myreadSBlocks(fd, sbuf) == -1) {
        return -1;
    }
    
    free_block = *((int *)(sbuf + 8));
    if (free_block <= 0) {
        fprintf(stderr, "No free blocks available\n");
        return -1;
    }
    
    return free_block;
}

/* Allocate a free block and update superblock */
int allocateBlock(int fd, int *block_num) {
    int free_block;
    int next_free = 0;
    
    /* Find a free block */
    free_block = findFreeBlock(fd);
    if (free_block == -1) {
        return -1;
    }
    
    /* Read the block to find the next free block */
    memset(buf, 0, BS);
    if (myreadBlock(fd, free_block, buf) == -1) {
        return -1;
    }
    
    /* The last 4 bytes contain the next free block */
    next_free = *((int *)(buf + BS - 4));
    
    /* Update superblock with next free block */
    if (myreadSBlocks(fd, sbuf) == -1) {
        return -1;
    }
    
    *((int *)(sbuf + 8)) = next_free;
    
    if (mywriteSBlocks(fd, sbuf) == -1) {
        return -1;
    }
    
    *block_num = free_block;
    return 0;
}

/* Mark a block as free */
void freeBlock(int fd, int block_num) {
    int current_free;
    
    /* Read superblock to get current free block */
    if (myreadSBlocks(fd, sbuf) == -1) {
        return;
    }
    
    current_free = *((int *)(sbuf + 8));
    
    /* Update the block to point to current free block */
    memset(buf, 0, BS);
    *((int *)(buf + BS - 4)) = current_free;
    
    if (mywriteBlock(fd, block_num, buf) == -1) {
        return;
    }
    
    /* Update superblock to point to this block */
    *((int *)(sbuf + 8)) = block_num;
    
    if (mywriteSBlocks(fd, sbuf) == -1) {
        return;
    }
}

/* Free a chain of blocks */
int freeChain(int fd, int first_block) {
    int next_block;
    int current_block = first_block;
    
    while (current_block != 0) {
        /* Read the current block */
        memset(buf, 0, BS);
        if (myreadBlock(fd, current_block, buf) == -1) {
            return -1;
        }
        
        /* Get the next block in the chain */
        next_block = *((int *)(buf + BS - 4));
        
        /* Free the current block */
        freeBlock(fd, current_block);
        
        /* Move to the next block */
        current_block = next_block;
    }
    
    return 0;
}

/* Parse a file path into components */
char** parseFilePath(const char *path, int *count) {
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));
    
    *count = 0;
    char **components = NULL;
    
    /* Handle root path specially */
    if (strcmp(path, "/") == 0) {
        components = malloc(sizeof(char*));
        components[0] = strdup("/");
        *count = 1;
        return components;
    }
    
    /* Split the path */
    char *token = strtok(path_copy, "/");
    while (token != NULL) {
        (*count)++;
        components = realloc(components, (*count) * sizeof(char*));
        components[(*count) - 1] = strdup(token);
        token = strtok(NULL, "/");
    }
    
    return components;
}

/* Free path components */
void freePathComponents(char **components, int count) {
    for (int i = 0; i < count; i++) {
        free(components[i]);
    }
    free(components);
}

/* Find a file or folder in the filesystem */
int findFile(int fd, const char *path, int *parent_block, int *file_offset, int *file_block) {
    int i, j;
    int current_block;
    int count;
    char **components;
    int found;
    
    /* Initialize return values */
    *parent_block = -1;
    *file_offset = -1;
    *file_block = -1;
    
    /* Read superblock to get root folder block */
    if (myreadSBlocks(fd, sbuf) == -1) {
        return -1;
    }
    
    current_block = *((int *)(sbuf + 12)); // Root folder block
    
    /* Handle root folder specially */
    if (strcmp(path, "/") == 0) {
        *parent_block = current_block;
        *file_offset = 0;
        *file_block = current_block;
        return 0;
    }
    
    /* Parse the path */
    components = parseFilePath(path, &count);
    if (components == NULL) {
        return -1;
    }
    
    /* Traverse the path */
    for (i = 0; i < count; i++) {
        found = 0;
        
        /* Read current folder block */
        memset(buf, 0, BS);
        if (myreadBlock(fd, current_block, buf) == -1) {
            freePathComponents(components, count);
            return -1;
        }
        
        /* Store parent block for the last component */
        if (i == count - 1) {
            *parent_block = current_block;
        }
        
        /* Search for the component in folder entries */
        /* Start at offset DESCRIPTOR_SIZE since the first entry is the folder itself */
        for (j = DESCRIPTOR_SIZE; j < BS - 4; j += DESCRIPTOR_SIZE) {
            if (buf[j] != 0 && strncmp(components[i], buf + j + 1, FNLEN) == 0) {
                /* Found the component */
                found = 1;
                
                /* Store file offset and block for the last component */
                if (i == count - 1) {
                    *file_offset = j;
                    *file_block = *((int *)(buf + j + 13));
                }
                
                /* Update current block for the next iteration */
                current_block = *((int *)(buf + j + 13));
                break;
            }
        }
        
        if (!found) {
            freePathComponents(components, count);
            return -1;
        }
    }
    
    freePathComponents(components, count);
    return 0;
}

/* Create a file or folder descriptor in a parent folder */
int createFileDescriptor(int fd, const char *filename, char type, int first_block, int size, int parent_block) {
    int i;
    int found = 0;
    
    /* Read parent folder block */
    memset(buf, 0, BS);
    if (myreadBlock(fd, parent_block, buf) == -1) {
        return -1;
    }
    
    /* Find an empty descriptor slot */
    for (i = DESCRIPTOR_SIZE; i < BS - 4; i += DESCRIPTOR_SIZE) {
        if (buf[i] == 0) {
            found = 1;
            break;
        }
    }
    
    if (!found) {
        fprintf(stderr, "Parent folder is full\n");
        return -1;
    }
    
    /* Create the descriptor */
    buf[i] = type;
    strncpy(buf + i + 1, filename, FNLEN);
    *((int *)(buf + i + 13)) = first_block;
    *((int *)(buf + i + 17)) = size;
    
    /* Write updated parent folder block */
    if (mywriteBlock(fd, parent_block, buf) == -1) {
        return -1;
    }
    
    return 0;
}

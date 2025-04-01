/* Filename: myfsv2.c */
/* Implements myfsv2 filesystem operations */
/* Compile using: gcc myfsv2.c -o mymkfs */
/* Then create links:
   ln mymkfs mycopyTo
   ln mymkfs mycopyFrom
   ln mymkfs myrm
   ln mymkfs mymkdir
   ln mymkfs myrmdir
   (myreadBlock and mystat are library functions, not commands)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>

#define MIN_BLOCK_SIZE 32
#define DESCRIPTOR_SIZE 21
#define MAX_FILENAME_LEN 12
#define TYPE_FILE 1
#define TYPE_DIR 2


typedef struct SuperBlock {
    int block_size;
    int num_blocks;       
    int root_dir_block;
    int first_free_block; 
} SuperBlock;

typedef struct Descriptor {
    char type;            
    char name[MAX_FILENAME_LEN]; 
    int first_block;
    int size;
} Descriptor;



/*Function Prototypes*/

int read_block(int fd, int block_num, void *buf, int block_size);
int write_block(int fd, int block_num, const void *buf, int block_size);
int read_superblock(int fd, SuperBlock *sb); // Assumes block 0, block_size 4096 initially to read size
int write_superblock(int fd, const SuperBlock *sb);
int allocate_block(int fd, SuperBlock *sb);
int free_block(int fd, SuperBlock *sb, int block_num);
int parse_path_spec(const char *full_path_spec, char **myfs_fname, char **internal_path);
int traverse_path(int fd, const SuperBlock *sb, const char *internal_path, Descriptor *result_desc, char **final_name_part_out);
int find_entry_in_dir(int fd, const SuperBlock *sb, int dir_start_block, const char *name, Descriptor *desc, int *containing_block, int *offset_in_block);
int add_entry_to_dir(int fd, SuperBlock *sb, int parent_dir_block, const Descriptor *new_desc);
int remove_entry_from_dir(int fd, const SuperBlock *sb, int parent_dir_block, const char *name);
int is_directory_empty(int fd, const SuperBlock *sb, int dir_start_block);


int mymkfs_impl(const char *fname, int block_size, int no_of_blocks);
int mycopyTo_impl(const char *linux_fname, char *myfs_path_spec);
int mycopyFrom_impl(char *myfs_path_spec, const char *linux_fname);
int myrm_impl(char *myfs_path_spec);
int mymkdir_impl(char *mydir_path_spec);
int myrmdir_impl(char *mydir_path_spec);
int myreadBlock_impl(char *myfname_spec, char *buf, int logical_block_no);
int mystat_impl(char *myname_spec, char *buf);


int main(int argc, char *argv[]) {
    char *basename_cmd;
    int ret = 0;

    basename_cmd = strrchr(argv[0], '/');
    if (basename_cmd != NULL) {
        basename_cmd++;
    } else {
        basename_cmd = argv[0];
    }

    if (strcmp(basename_cmd, "mymkfs") == 0) {
if (argc != 4) {
        fprintf(stderr, "Usage: %s <myfs_filename> <block_size> <num_blocks>\n", argv[0]);
        exit(1);
}
        int block_size = atoi(argv[2]);
        int num_blocks = atoi(argv[3]);
        ret = mymkfs_impl(argv[1], block_size, num_blocks);
    } 
    else if (strcmp(basename_cmd, "mycopyTo") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s <linux_filename> <path/to/myfile@myfs_filename>\n", argv[0]);
            exit(1);
        }
        ret = mycopyTo_impl(argv[1], argv[2]);
    } else if (strcmp(basename_cmd, "mycopyFrom") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s <path/to/myfile@myfs_filename> <linux_filename>\n", argv[0]);
            exit(1);
        }
        ret = mycopyFrom_impl(argv[1], argv[2]);
    } else if (strcmp(basename_cmd, "myrm") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Usage: %s <path/to/myfile@myfs_filename>\n", argv[0]);
            exit(1);
        }
        ret = myrm_impl(argv[1]);
    } else if (strcmp(basename_cmd, "mymkdir") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Usage: %s <path/to/mydir@myfs_filename>\n", argv[0]);
            exit(1);
        }
        ret = mymkdir_impl(argv[1]);
    } else if (strcmp(basename_cmd, "myrmdir") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Usage: %s <path/to/mydir@myfs_filename>\n", argv[0]);
            exit(1);
        }
        ret = myrmdir_impl(argv[1]);
    } else {
        fprintf(stderr, "%s: Command not found (executable name should be one of mymkfs, mycopyTo, etc.)\n", argv[0]);
        exit(1);
    }

    if (ret != 0) {
        fprintf(stderr, "%s failed.\n", basename_cmd);
        exit(1);
    }

    return 0; 
}







int read_block(int fd, int block_num, void *buf, int block_size) {
    off_t offset = (off_t)block_num * block_size;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek failed in read_block");
        return -1;
    }
    ssize_t bytes_read = read(fd, buf, block_size);
    if (bytes_read == -1) {
        perror("read failed in read_block");
        return -1;
    }
    if (bytes_read < block_size) {
         // This may happen if reading past EOF, treat as error for FS consistency unless block_num is 0.
        fprintf(stderr, "Error: Short read in read_block for block %d (expected %d, got %zd)\n", block_num, block_size, bytes_read);
        return -1;
    }
    return 0;
}

int write_block(int fd, int block_num, const void *buf, int block_size) {
    off_t offset = (off_t)block_num * block_size;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek failed in write_block");
        return -1;
    }
    ssize_t bytes_written = write(fd, buf, block_size);
    if (bytes_written == -1) {
        perror("write failed in write_block");
        return -1;
    }
    if (bytes_written < block_size) {
        fprintf(stderr, "Error: Short write in write_block for block %d (expected %d, got %zd)\n", block_num, block_size, bytes_written);
        // This indicates a serious problem, like disk full mid-write
        return -1; // Treat as an error
    }
    return 0;
}

int read_superblock(int fd, SuperBlock *sb) {
    int initial_block_size_read;
    // Temporarily read just the first int to find the block_size
    if (lseek(fd, 0, SEEK_SET) == -1) {
         perror("lseek failed in read_superblock (initial)");
         return -1;
    }
    if (read(fd, &initial_block_size_read, sizeof(int)) != sizeof(int)) {
        struct stat st;
        if (fstat(fd, &st) == 0 && st.st_size == 0) {
             fprintf(stderr, "Error: Cannot read superblock from empty file.\n");
        } else {
             perror("Error reading block size from superblock");
        }
        return -1;
    }
     if (initial_block_size_read < MIN_BLOCK_SIZE) {
         fprintf(stderr, "Error: Invalid block size %d read from superblock (must be >= %d)\n", initial_block_size_read, MIN_BLOCK_SIZE);
         return -1;
     }

    char *temp_buf = malloc(initial_block_size_read);
    if (!temp_buf) {
        perror("malloc failed for superblock buffer");
        return -1;
    }
    if (lseek(fd, 0, SEEK_SET) == -1) {
         perror("lseek failed in read_superblock (full)");
         free(temp_buf);
         return -1;
    }
    ssize_t bytes_read = read(fd, temp_buf, initial_block_size_read);
     if (bytes_read == -1) {
        perror("read failed in read_superblock (full)");
        free(temp_buf);
        return -1;
    }
    if (bytes_read < initial_block_size_read) {
        fprintf(stderr, "Error: Short read for full superblock (block 0) (expected %d, got %zd)\n", initial_block_size_read, bytes_read);
        free(temp_buf);
        return -1;
    }


    memcpy(sb, temp_buf, sizeof(SuperBlock));
    free(temp_buf);

    if (sb->block_size != initial_block_size_read) {
        fprintf(stderr, "Warning: Block size mismatch in superblock struct vs initial read (%d vs %d)\n", sb->block_size,
             initial_block_size_read);
         return -1;
    }
     if (sb->root_dir_block <= 0 || sb->root_dir_block > sb->num_blocks) {
         fprintf(stderr, "Error: Invalid root directory block number %d in superblock (num_blocks=%d)\n", sb->root_dir_block,
             sb->num_blocks);
         return -1;
     }
     if (sb->first_free_block < 0 || sb->first_free_block > sb->num_blocks) {
         fprintf(stderr, "Error: Invalid first free block number %d in superblock (num_blocks=%d)\n", sb->first_free_block,
             sb->num_blocks);
         return -1;
     }

    return 0;
}

int write_superblock(int fd, const SuperBlock *sb) {
    if (sb->block_size < MIN_BLOCK_SIZE) {
         fprintf(stderr, "Error: Attempting to write invalid block size %d to superblock\n", sb->block_size);
         return -1;
     }
    char *temp_buf = calloc(1, sb->block_size);
    if (!temp_buf) {
        perror("calloc failed for superblock buffer");
        return -1;
    }
    memcpy(temp_buf, sb, sizeof(SuperBlock));

    if (write_block(fd, 0, temp_buf, sb->block_size) != 0) {
        fprintf(stderr, "Failed to write superblock (block 0)\n");
        free(temp_buf);
        return -1;
    }

    free(temp_buf);
    return 0;
}


int allocate_block(int fd, SuperBlock *sb) {
    if (sb->first_free_block == 0) {
        fprintf(stderr, "Filesystem full: No free blocks available.\n");
        return -1;
    }

    int allocated_block_num = sb->first_free_block;
    char *block_buf = malloc(sb->block_size);
    if (!block_buf) {
        perror("malloc failed in allocate_block");
        return -1;
    }

    // Read the block being allocated to get the *next* free block pointer
    if (read_block(fd, allocated_block_num, block_buf, sb->block_size) != 0) {
        fprintf(stderr, "Error reading free block %d during allocation\n", allocated_block_num);
        free(block_buf);
        return -1;
    }

/*The next free block number is stored in the last 4 bytes */
    int next_free_block;
    memcpy(&next_free_block, block_buf + sb->block_size - sizeof(int), sizeof(int));

// Update the superblock IN MEMORY first
sb->first_free_block = next_free_block;

// Write the updated superblock to disk
    if (write_superblock(fd, sb) != 0) {
        fprintf(stderr, "Error writing superblock after block allocation\n");
        sb->first_free_block = allocated_block_num;
        free(block_buf);
        return -1;
}


free(block_buf);
return allocated_block_num;
}

int free_block(int fd, SuperBlock *sb, int block_num) {
    if (block_num <= 0 || block_num > sb->num_blocks) {
        fprintf(stderr, "Error: Attempt to free invalid block number %d (must be 1..%d)\n", block_num, sb->num_blocks);
        return -1;
    }
     if (block_num == sb->root_dir_block) {
         fprintf(stderr, "Warning: Attempt to free root directory block %d. Cannot Remove Directory.\n", block_num);
     }


    char *block_buf = malloc(sb->block_size);
    if (!block_buf) {
        perror("malloc failed in free_block");
        return -1;
    }
    memset(block_buf, 0, sb->block_size);

    int current_head = sb->first_free_block;
    memcpy(block_buf + sb->block_size - sizeof(int), &current_head, sizeof(int));

    if (write_block(fd, block_num, block_buf, sb->block_size) != 0) {
        fprintf(stderr, "Error writing block %d during free operation\n", block_num);
        free(block_buf);
        return -1;
    }
    free(block_buf);

    sb->first_free_block = block_num;

    // Write the updated superblock to disk
    if (write_superblock(fd, sb) != 0) {
        fprintf(stderr, "Error writing superblock after freeing block %d\n", block_num);
        sb->first_free_block = current_head;
        return -1;
    }

    return 0;
}

int parse_path_spec(const char *full_path_spec, char **myfs_fname, char **internal_path) {
    char *at_symbol = strrchr(full_path_spec, '@');
    if (at_symbol == NULL) {
        fprintf(stderr, "Invalid format: Missing '@' in path specification '%s'\n", full_path_spec);
        return -1;
    }

    // Use strndup to avoid modifying the original string and handle allocation
    *internal_path = strndup(full_path_spec, at_symbol - full_path_spec);
    if (*internal_path == NULL) {
        perror("strndup failed for internal path");
        return -1;
    }

    *myfs_fname = strdup(at_symbol + 1);
    if (*myfs_fname == NULL) {
        perror("strdup failed for myfs filename");
        free(*internal_path); // Clean up previous allocation
        *internal_path = NULL;
        return -1;
    }

    if (strlen(*myfs_fname) == 0) {
        fprintf(stderr, "Invalid format: Missing myfs filename after '@' in '%s'\n", full_path_spec);
        free(*internal_path);
        free(*myfs_fname);
        *internal_path = NULL;
        *myfs_fname = NULL;
        return -1;
    }
    // Allow empty internal path [refers to root content]
     // Ensuring internal path starts with / if not empty
     // This simplifies traversal logic slightly.
    if (strlen(*internal_path) > 0 && (*internal_path)[0] != '/') {
         char *temp = malloc(strlen(*internal_path) + 2); // Space for '/' and '\0'
         if(!temp) {
             perror("malloc failed for path adjustment");
             free(*internal_path); free(*myfs_fname); return -1;
         }
         sprintf(temp, "/%s", *internal_path);
         free(*internal_path);
         *internal_path = temp;
    }
     else if (strlen(*internal_path) == 0) {
         // Standardize empty path to "/"
         free(*internal_path);
         *internal_path = strdup("/");
          if(!*internal_path) {
             perror("strdup failed for root path");
             free(*myfs_fname); return -1;
          }
     }


    return 0;
}


// Finds an entry within a specific directory's block chain.
int find_entry_in_dir(int fd, const SuperBlock *sb, int dir_start_block, const char *name, Descriptor *desc, int *containing_block, int *offset_in_block) {
    int current_block_num = dir_start_block;
    char *dir_buf = malloc(sb->block_size);
    if (!dir_buf) {
        perror("malloc failed in find_entry_in_dir");
        return -2; 
    }

    while (current_block_num != 0) {
        if (read_block(fd, current_block_num, dir_buf, sb->block_size) != 0) {
            fprintf(stderr, "Error reading directory block %d\n", current_block_num);
            free(dir_buf);
            return -2;
        }

        int num_descriptors_per_block = (sb->block_size - sizeof(int)) / DESCRIPTOR_SIZE;
         if (DESCRIPTOR_SIZE == 0) num_descriptors_per_block = 0; 
         else num_descriptors_per_block = (sb->block_size - sizeof(int)) / DESCRIPTOR_SIZE;

        for (int i = 0; i < num_descriptors_per_block; ++i) {
            int current_offset = i * DESCRIPTOR_SIZE;
            Descriptor *current_desc = (Descriptor *)(dir_buf + current_offset);

            if (current_desc->type != 0 && strncmp(current_desc->name, name, MAX_FILENAME_LEN) == 0) {
                if (desc) {
                    memcpy(desc, current_desc, sizeof(Descriptor));
                }
                if (containing_block) {
                    *containing_block = current_block_num;
                }
                if (offset_in_block) {
                    *offset_in_block = current_offset;
                }
                free(dir_buf);
                return 0; // Found
            }
        }

        memcpy(&current_block_num, dir_buf + sb->block_size - sizeof(int), sizeof(int)); // Fixed: & added
    }

    free(dir_buf);
    return -1; 
}


int traverse_path(int fd, const SuperBlock *sb, const char *internal_path, Descriptor *result_desc, char **final_name_part_out) {
    char *path_copy = strdup(internal_path); // Need mutable copy for strtok
    if (!path_copy) { perror("strdup failed in traverse_path"); return -5; }

    int current_dir_block = sb->root_dir_block;
    int parent_dir_block = sb->root_dir_block; // Root's parent is root for this logic
    char *final_name_part = NULL; // Store the last component name
    char *last_found_name = NULL; // Store the name of the last component successfully found

    // Handle root case explicitly first
    if (strcmp(path_copy, "/") == 0) {
         if (result_desc) { // Create a pseudo-descriptor for root
             result_desc->type = TYPE_DIR;
             strncpy(result_desc->name, "/", MAX_FILENAME_LEN);
             result_desc->name[MAX_FILENAME_LEN -1] = '\0';
             result_desc->first_block = sb->root_dir_block;
             result_desc->size = 0; // Size maybe irrelevant for root dir itself
         }
         if (final_name_part_out) *final_name_part_out = strdup("/"); // Caller must free
         free(path_copy);
         return 0; // Indicate root itself was the target
    }

    // Use strtok_r for reentrancy (good practice)
    char *token;
    char *saveptr; // for strtok_r
    char *path_to_tokenize = path_copy;

    // Skip leading '/' if present (should be, based on parse_path_spec adjustment)
    if (path_to_tokenize[0] == '/') path_to_tokenize++;

    Descriptor current_entry_desc;
    int find_status;

    token = strtok_r(path_to_tokenize, "/", &saveptr);

    while (token != NULL) {
         if (strlen(token) == 0) { // Handle consecutive slashes "//" - shouldn't happen if root handled
             token = strtok_r(NULL, "/", &saveptr);
             continue;
         }
         if (strlen(token) >= MAX_FILENAME_LEN) {
              fprintf(stderr, "Error: Path component '%s' exceeds maximum length (%d)\n", token, MAX_FILENAME_LEN-1);
              free(path_copy);
              return -5; // Or a specific error code for bad path component
         }

         // Find the token in the current_dir_block
         find_status = find_entry_in_dir(fd, sb, current_dir_block, token, &current_entry_desc, NULL, NULL); // Fixed: & added

         if (find_status == 0) { // Found the token
             // Record the name we just found
             if(last_found_name) free(last_found_name);
             last_found_name = strdup(token);

             char* next_token = strtok_r(NULL, "/", &saveptr);

             if (next_token != NULL) { // More path components follow
                 if (current_entry_desc.type == TYPE_DIR) {
                     // It's a directory, descend into it
                     parent_dir_block = current_dir_block; // Update parent before descending
                     current_dir_block = current_entry_desc.first_block;
                     token = next_token; // Continue loop with the next token
                 } else {
                     // It's a file, but not the last component. Error.
                     fprintf(stderr, "Error: Cannot traverse through file '%s' in path '%s'\n", token, internal_path);
                     if(last_found_name) free(last_found_name);
                     free(path_copy);
                     return -2; // Invalid path component type
                 }
             } else { // This was the last token and it was found
                 if (result_desc) memcpy(result_desc, &current_entry_desc, sizeof(Descriptor)); // Fixed: & added
                 if (final_name_part_out) *final_name_part_out = strdup(token); // Caller must free
                 if(last_found_name) free(last_found_name);
                 free(path_copy);
                 // Return the block number of the directory containing the found entry
                 return current_dir_block;
             }
         } else if (find_status == -1) { // Token not found in current directory
             char* next_token = strtok_r(NULL, "/", &saveptr);
             if (next_token != NULL) { // An intermediate component was missing
                 fprintf(stderr, "Error: Path component '%s' not found in '%s'\n", token, internal_path);
                 if(last_found_name) free(last_found_name);
                 free(path_copy);
                 return -1; // Intermediate component missing
             } else {
                 // It was the last token and it wasn't found. This is okay for creation ops.
                 if (final_name_part_out) *final_name_part_out = strdup(token); // Caller must free
                 if(last_found_name) free(last_found_name);
                 free(path_copy);
                 // Return the block number of the directory where it *should* be created
                 return current_dir_block;
             }
         } else { // find_entry_in_dir reported an error (-2)
             fprintf(stderr, "Error reading directory block while searching for '%s'\n", token);
             if(last_found_name) free(last_found_name);
             free(path_copy);
             return -3; // Error reading block
         }
    }

    // Should only reach here if the path was just "/" (handled at the start) or
    // if the path ended with slashes like "/dir1/dir2///"
    // In the latter case, the last *found* component was a directory.
    // We need to return its parent block and its name.

    // This case indicates the path successfully resolved to an existing directory
    // *before* the last token was processed (or the last token was empty).
    // Example: path is "/dir1/", token loop ends after processing "dir1".
    // current_dir_block is dir1's data block. parent_dir_block is root's block.
    // We need the descriptor of "dir1".

    if (last_found_name) { // We successfully found at least one component
         // Re-find the descriptor for the last successfully found component in its parent
         Descriptor last_dir_desc;
         if (find_entry_in_dir(fd, sb, parent_dir_block, last_found_name, &last_dir_desc, NULL, NULL) == 0) {
              if (result_desc) memcpy(result_desc, &last_dir_desc, sizeof(Descriptor));
         } else {
              fprintf(stderr,"Internal inconsistency: couldn't re-find last dir %s\n", last_found_name);
              // Fall through might be okay, but indicates logic issue
         }
         if (final_name_part_out) *final_name_part_out = strdup(last_found_name); // Return name of the final dir found
         free(last_found_name);
         free(path_copy);
         return parent_dir_block; // Return the parent of the final dir found
    } else {
         // This case should technically not be reached due to root handling and path parsing adjustments
         fprintf(stderr, "Internal error: traverse_path ended unexpectedly for path '%s'\n", internal_path);
         free(path_copy);
         return -5;
    }
}


// Adds an entry to a directory chain. Allocates new blocks if needed.
int add_entry_to_dir(int fd, SuperBlock *sb, int parent_dir_block, const Descriptor *new_desc) {
    int current_block_num = parent_dir_block;
    int last_block_num = parent_dir_block; // Keep track of the last block in case we need to extend
    char *dir_buf = malloc(sb->block_size);
    if (!dir_buf) { perror("malloc failed in add_entry_to_dir"); return -1; }
    // int found_slot = 0; // Unused variable

    while (current_block_num != 0) {
        last_block_num = current_block_num;
        if (read_block(fd, current_block_num, dir_buf, sb->block_size) != 0) {
            fprintf(stderr, "Error reading directory block %d while adding entry\n", current_block_num);
            free(dir_buf); return -1;
        }

        int num_descriptors_per_block;
         if (DESCRIPTOR_SIZE == 0) num_descriptors_per_block = 0; // Avoid division by zero
         else num_descriptors_per_block = (sb->block_size - sizeof(int)) / DESCRIPTOR_SIZE;

        for (int i = 0; i < num_descriptors_per_block; ++i) {
            int current_offset = i * DESCRIPTOR_SIZE;
            Descriptor *current_desc_ptr = (Descriptor *)(dir_buf + current_offset);

            // Check if descriptor slot is free (type == 0)
            if (current_desc_ptr->type == 0) {
                // Found an empty slot, write the new descriptor here
                memcpy(current_desc_ptr, new_desc, sizeof(Descriptor));
                if (write_block(fd, current_block_num, dir_buf, sb->block_size) != 0) {
                    fprintf(stderr, "Error writing directory block %d after adding entry\n", current_block_num);
                    // Revert the memcpy? Difficult. Best to just return error.
                    free(dir_buf); return -1;
                }
                // Optional: Update parent directory size in its descriptor? Spec unclear, maybe not needed.
                free(dir_buf);
                return 0; // Success
            }
        }
        // No empty slot in this block, move to the next block in the chain
        memcpy(&current_block_num, dir_buf + sb->block_size - sizeof(int), sizeof(int)); // Fixed: & added
    }

    // If we exit the loop, it means no empty slot was found in the entire chain.
    // We need to allocate a new block and add it to the chain.
    int new_block_num = allocate_block(fd, sb);
    if (new_block_num == -1) {
        fprintf(stderr, "Failed to allocate new block for directory extension\n");
        free(dir_buf); return -1;
    }

    // Link the *last* block of the existing chain to this new block
    // We still have the buffer for the 'last_block_num' in dir_buf (read in the last loop iteration)
    memcpy(dir_buf + sb->block_size - sizeof(int), &new_block_num, sizeof(int));
    if (write_block(fd, last_block_num, dir_buf, sb->block_size) != 0) {
        fprintf(stderr, "Error linking last directory block %d to new block %d\n", last_block_num, new_block_num);
        // Try to free the allocated block to avoid leaks, but state is inconsistent
        free_block(fd, sb, new_block_num); // Best effort rollback
        free(dir_buf); return -1;
    }

    // Initialize the new block (zero it out, including the next pointer)
    memset(dir_buf, 0, sb->block_size);
    // Add the new descriptor to the *first* slot of the new block
    memcpy(dir_buf, new_desc, sizeof(Descriptor));
    // Write the new block
    if (write_block(fd, new_block_num, dir_buf, sb->block_size) != 0) {
        fprintf(stderr, "Error writing initial data to new directory block %d\n", new_block_num);
        // State is inconsistent. Last block points here, but this block is bad.
        // Hard to fully rollback the link written previously.
        free(dir_buf); return -1;
    }

    free(dir_buf);
    return 0; // Success
}

// Removes an entry from a directory chain (marks as empty). Does NOT free data blocks.
int remove_entry_from_dir(int fd, const SuperBlock *sb, int parent_dir_block, const char *name) {
     int current_block_num = parent_dir_block;
     char *dir_buf = malloc(sb->block_size);
     if (!dir_buf) { perror("malloc failed in remove_entry_from_dir"); return -2; }

     while (current_block_num != 0) {
         if (read_block(fd, current_block_num, dir_buf, sb->block_size) != 0) {
             fprintf(stderr, "Error reading directory block %d while removing entry\n", current_block_num);
             free(dir_buf); return -2;
         }

         int num_descriptors_per_block;
          if (DESCRIPTOR_SIZE == 0) num_descriptors_per_block = 0; // Avoid division by zero
          else num_descriptors_per_block = (sb->block_size - sizeof(int)) / DESCRIPTOR_SIZE;

         for (int i = 0; i < num_descriptors_per_block; ++i) {
             int current_offset = i * DESCRIPTOR_SIZE;
             Descriptor *current_desc_ptr = (Descriptor *)(dir_buf + current_offset);

             // Check if descriptor is valid and name matches
             if (current_desc_ptr->type != 0 && strncmp(current_desc_ptr->name, name, MAX_FILENAME_LEN) == 0) {
                 // Found the entry, mark it as deleted (set type to 0)
                 // Keep other fields? Setting type to 0 is sufficient. Maybe clear name for tidiness.
                 current_desc_ptr->type = 0;
                 memset(current_desc_ptr->name, 0, MAX_FILENAME_LEN); // Optional cleanup
                 // current_desc_ptr->first_block = 0; // Optional cleanup
                 // current_desc_ptr->size = 0; // Optional cleanup


                 if (write_block(fd, current_block_num, dir_buf, sb->block_size) != 0) {
                     fprintf(stderr, "Error writing directory block %d after removing entry '%s'\n", current_block_num, name);
                     free(dir_buf); return -2;
                 }
                 // Optional: Check if block is now completely empty and can be removed from chain/freed?
                 // This adds significant complexity (needs prev block pointer or careful relinking).
                 // Current approach leaves empty slots, which is simpler and common.
                 free(dir_buf);
                 return 0; // Success
             }
         }
         // Not found in this block, move to the next
         memcpy(&current_block_num, dir_buf + sb->block_size - sizeof(int), sizeof(int)); // Fixed: & added
     }

     free(dir_buf);
     return -1; // Not found
}


// Checks if a directory is empty.
int is_directory_empty(int fd, const SuperBlock *sb, int dir_start_block) {
    int current_block_num = dir_start_block;
    char *dir_buf = malloc(sb->block_size);
    if (!dir_buf) { perror("malloc failed in is_directory_empty"); return -1; }

    while (current_block_num != 0) {
        if (read_block(fd, current_block_num, dir_buf, sb->block_size) != 0) {
            fprintf(stderr, "Error reading directory block %d while checking if empty\n", current_block_num);
            free(dir_buf); return -1; // Indicate error
        }

        int num_descriptors_per_block;
         if (DESCRIPTOR_SIZE == 0) num_descriptors_per_block = 0; // Avoid division by zero
         else num_descriptors_per_block = (sb->block_size - sizeof(int)) / DESCRIPTOR_SIZE;

        for (int i = 0; i < num_descriptors_per_block; ++i) {
            int current_offset = i * DESCRIPTOR_SIZE;
            Descriptor *current_desc_ptr = (Descriptor *)(dir_buf + current_offset);

            // Check if descriptor slot is used (type != 0)
            if (current_desc_ptr->type != 0) {
                // Found a valid entry, directory is not empty
                free(dir_buf);
                return 0; // Not empty
            }
        }
        // No entries in this block, check next
        memcpy(&current_block_num, dir_buf + sb->block_size - sizeof(int), sizeof(int)); // Fixed: & added
    }

    // If we finish the loop, no entries were found
    free(dir_buf);
    return 1; // Empty
}


// --- Filesystem Operation Implementations ---

int mymkfs_impl(const char *fname, int block_size, int no_of_blocks) {
    if (block_size < MIN_BLOCK_SIZE) {
        fprintf(stderr, "Error: Block size %d is too small (minimum %d required).\n", block_size, MIN_BLOCK_SIZE);
        return -1;
    }
    if (block_size % 4 != 0) {
        // While not strictly necessary for this impl, aligned block sizes are conventional
        fprintf(stderr, "Warning: Block size %d is not a multiple of 4. This might be inefficient.\n", block_size);
    }
    if (no_of_blocks < 1) { // Need at least 1 block for root directory data
        fprintf(stderr, "Error: Number of data blocks must be at least 1 (for root directory).\n");
        return -1;
    }

    int fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        perror("Error opening file for mymkfs");
        return -1;
    }

    // Calculate total size and truncate the file
    // Total blocks = 1 (SB) + no_of_blocks (Data)
    off_t total_size = (off_t)(no_of_blocks + 1) * block_size;
    if (ftruncate(fd, total_size) == -1) {
        perror("Error truncating file for mymkfs");
        close(fd);
        unlink(fname); // Attempt to remove the partially created file
        return -1;
    }

    // Prepare Super Block
    SuperBlock sb;
    memset(&sb, 0, sizeof(SuperBlock)); // Zero out structure
    sb.block_size = block_size;
    sb.num_blocks = no_of_blocks; // Number of *data* blocks
    sb.root_dir_block = 1; // Root directory data starts at block 1 (relative to start of FS)
    sb.first_free_block = (no_of_blocks >= 2) ? 2 : 0; // Free list starts at block 2 if available (Blocks: 0=SB, 1=Root, 2=Free...)

    // Write Super Block (Block 0)
    if (write_superblock(fd, &sb) != 0) {
        fprintf(stderr, "Error writing superblock during mkfs\n");
        close(fd); unlink(fname); return -1;
    }

    // Initialize Root Directory (Block 1)
    char *block_buf = calloc(1, block_size); // Use calloc to zero out
    if (!block_buf) { perror("calloc failed for root dir buffer"); close(fd); unlink(fname); return -1; }
    // Next pointer is already 0 due to calloc, indicating end of chain for root dir initially
    if (write_block(fd, sb.root_dir_block, block_buf, block_size) != 0) {
        fprintf(stderr, "Error writing root directory block during mkfs\n");
        free(block_buf); close(fd); unlink(fname); return -1;
    }

    // Initialize Free Block List (Blocks 2 to no_of_blocks)
    // Block i points to block i+1, last data block points to 0.
    for (int i = 2; i <= no_of_blocks; ++i) {
        memset(block_buf, 0, block_size); // Clear buffer
        int next_free = (i == no_of_blocks) ? 0 : i + 1; // Point to next block or 0 if last data block
        memcpy(block_buf + block_size - sizeof(int), &next_free, sizeof(int));
        if (write_block(fd, i, block_buf, block_size) != 0) {
            fprintf(stderr, "Error writing free block %d during mkfs\n", i);
            free(block_buf); close(fd); unlink(fname); return -1;
        }
    }

    free(block_buf);
    if (close(fd) == -1) {
        perror("Error closing file after mymkfs");
        // File is likely created, but closing failed. Not removing it.
        return -1; // Return error even if writes seemed ok
    }
    printf("Filesystem created successfully on %s (%d data blocks of %d bytes).\n", fname, no_of_blocks, block_size);
    return 0;
}


int mycopyTo_impl(const char *linux_fname, char *myfs_path_spec) {
    char *myfs_fname = NULL;
    char *internal_path = NULL;
    char *target_name = NULL;
    int parent_dir_block;
    // Descriptor target_desc; // Not needed here, traverse just gives parent block
    SuperBlock sb;
    int fd = -1, linux_fd = -1;
    int first_data_block = -1; // Track first allocated block for potential rollback
    int blocks_allocated_count = 0; // Track all allocated blocks for rollback
    int *allocated_blocks = NULL;   // Array to store allocated block numbers
    int allocated_blocks_capacity = 10; // Initial capacity
    char *data_buf = NULL;
    int result = -1; // Default to error
    int entry_added = 0; // Flag to check if dir entry was added for rollback

    allocated_blocks = malloc(allocated_blocks_capacity * sizeof(int));
    if (!allocated_blocks) { perror("malloc failed for rollback info"); goto cleanup; }


    // 1. Parse paths
    if (parse_path_spec(myfs_path_spec, &myfs_fname, &internal_path) != 0) {
        goto cleanup; // Error message already printed
    }

    // 2. Stat linux file
    struct stat linux_stat;
    if (stat(linux_fname, &linux_stat) == -1) {
        perror("stat failed for source linux file");
        goto cleanup;
    }
     if (!S_ISREG(linux_stat.st_mode)) {
         fprintf(stderr, "Error: Source '%s' is not a regular file.\n", linux_fname);
         goto cleanup;
     }
     if (linux_stat.st_size < 0) {
         fprintf(stderr, "Error: Source file '%s' has invalid size %ld.\n", linux_fname, (long)linux_stat.st_size);
         goto cleanup;
     }


    // 3. Open myfs file and read superblock
    fd = open(myfs_fname, O_RDWR);
    if (fd == -1) {
        perror("Error opening myfs file");
        goto cleanup;
    }
    if (read_superblock(fd, &sb) != 0) {
        fprintf(stderr, "Error reading superblock from %s\n", myfs_fname);
        goto cleanup;
    }
    data_buf = malloc(sb.block_size);
    if (!data_buf) { perror("malloc failed for data buffer"); goto cleanup;}


    // 4. Traverse path to find parent directory block and target name
    int traverse_status = traverse_path(fd, &sb, internal_path, NULL, &target_name);

    if (traverse_status < 0) { // Error during traversal (-1, -2, -3, -5)
        fprintf(stderr, "Error traversing path '%s' in %s\n", internal_path, myfs_fname);
        // Specific error should be printed by traverse_path
        goto cleanup;
    }
     if (traverse_status == 0) { // Path was root "/"
         // We need the actual root block number to check for existence inside root
         parent_dir_block = sb.root_dir_block;
     } else {
         parent_dir_block = traverse_status;
     }

     if (!target_name || strcmp(target_name, "/") == 0) {
         fprintf(stderr, "Error: Invalid target name derived from path '%s'. Cannot copy into root itself.\n", internal_path);
         goto cleanup;
     }


    // 5. Check if target name already exists in parent directory
    Descriptor existing_desc;
    if (find_entry_in_dir(fd, &sb, parent_dir_block, target_name, &existing_desc, NULL, NULL) == 0) {
        fprintf(stderr, "Error: Target '%s' already exists in the destination directory of %s\n", target_name, myfs_path_spec);
        goto cleanup;
    }


    // 6. Check filename length
    if (strlen(target_name) >= MAX_FILENAME_LEN) {
        fprintf(stderr, "Error: Target filename '%s' is too long (max %d chars).\n", target_name, MAX_FILENAME_LEN -1);
        goto cleanup;
    }

    // 7. Open linux file for reading
    linux_fd = open(linux_fname, O_RDONLY);
    if (linux_fd == -1) {
        perror("Error opening source linux file for reading");
        goto cleanup;
    }

    // 8. Allocate first block for the file data (if file size > 0)
    if (linux_stat.st_size > 0) {
        first_data_block = allocate_block(fd, &sb);
        if (first_data_block == -1) {
            fprintf(stderr, "Error allocating first data block (disk full?)\n");
            goto cleanup;
        }
        allocated_blocks[blocks_allocated_count++] = first_data_block;
    } else {
        first_data_block = 0; // Special case: 0-byte file has no data blocks
    }


    // 9. Create descriptor for the new file
    Descriptor new_file_desc;
    memset(&new_file_desc, 0, sizeof(Descriptor));
    new_file_desc.type = TYPE_FILE;
    strncpy(new_file_desc.name, target_name, MAX_FILENAME_LEN);
    new_file_desc.name[MAX_FILENAME_LEN - 1] = '\0'; // Ensure null termination
    new_file_desc.first_block = first_data_block;
    new_file_desc.size = linux_stat.st_size;

    // 10. Add descriptor to parent directory
    if (add_entry_to_dir(fd, &sb, parent_dir_block, &new_file_desc) != 0) {
        fprintf(stderr, "Error adding file entry to directory\n");
        // Rollback: free the allocated block(s)
        goto rollback;
    }
    entry_added = 1; // Mark that entry was added

    // 11. Copy data block by block (only if size > 0)
    if (linux_stat.st_size > 0) {
        ssize_t bytes_read;
        off_t bytes_copied = 0;
        int current_myfs_block = first_data_block;
        int next_myfs_block = 0;
        int data_bytes_per_block = sb.block_size - sizeof(int);

        while ((bytes_read = read(linux_fd, data_buf, data_bytes_per_block)) > 0) {
            bytes_copied += bytes_read;

            // Is this the last chunk of data?
            if (bytes_copied >= linux_stat.st_size) {
                next_myfs_block = 0; // Mark end of chain
                // Zero out remaining part of buffer if bytes_read < capacity
                if (bytes_read < data_bytes_per_block) {
                    memset(data_buf + bytes_read, 0, data_bytes_per_block - bytes_read);
                }
            } else {
                // Need another block
                next_myfs_block = allocate_block(fd, &sb);
                if (next_myfs_block == -1) {
                    fprintf(stderr, "Error allocating data block during copy (disk full?)\n");
                    goto rollback; // Rollback needed
                }
                 // Grow allocated_blocks array if needed
                 if (blocks_allocated_count >= allocated_blocks_capacity) {
                     allocated_blocks_capacity *= 2;
                     int *temp = realloc(allocated_blocks, allocated_blocks_capacity * sizeof(int));
                     if (!temp) { perror("realloc failed for rollback info"); goto rollback; } // Critical, cannot track
                     allocated_blocks = temp;
                 }
                 allocated_blocks[blocks_allocated_count++] = next_myfs_block;

            }

            // Write next block pointer to end of current buffer
            memcpy(data_buf + sb.block_size - sizeof(int), &next_myfs_block, sizeof(int));

            // Write current data block
            if (write_block(fd, current_myfs_block, data_buf, sb.block_size) != 0) {
                fprintf(stderr, "Error writing data block %d during copy\n", current_myfs_block);
                goto rollback; // Rollback needed
            }

            // Move to next block
            current_myfs_block = next_myfs_block;
            if (current_myfs_block == 0) break; // Finished writing data chain
        } // end while read loop

        if (bytes_read == -1) {
            perror("Error reading from source linux file during copy");
            goto rollback; // Rollback needed
        }
         if (bytes_copied != linux_stat.st_size) {
              fprintf(stderr, "Warning: Copied %lld bytes, but linux file size was %lld. Copy may be incomplete.\n",
                      (long long)bytes_copied, (long long)linux_stat.st_size);
             // Proceed but maybe flag error? Let's consider it an error state.
             goto rollback;
         }
    } // end if size > 0


    result = 0; // Success
    goto cleanup; // Skip rollback

rollback:
    fprintf(stderr, "Error occurred, attempting rollback...\n");
    result = -1; // Ensure error status

    // Rollback 1: Remove directory entry if it was added
    if (entry_added && fd != -1 && target_name) {
         // Need to re-read SB in case free_block changed it during allocation attempts
         SuperBlock sb_rollback;
         if (read_superblock(fd, &sb_rollback) == 0) {
             if (remove_entry_from_dir(fd, &sb_rollback, parent_dir_block, target_name) != 0) {
                 fprintf(stderr, "Rollback warning: Failed to remove directory entry for '%s'. Filesystem inconsistent.\n", target_name);
             } else {
                  fprintf(stderr, "Rollback: Removed directory entry for '%s'.\n", target_name);
             }
         } else {
             fprintf(stderr, "Rollback warning: Failed to re-read superblock, cannot remove directory entry.\n");
         }
    }

    // Rollback 2: Free any allocated blocks
    if (blocks_allocated_count > 0 && fd != -1) {
        SuperBlock sb_rollback; // Read SB again
         if (read_superblock(fd, &sb_rollback) == 0) {
             fprintf(stderr, "Rollback: Freeing %d allocated blocks...\n", blocks_allocated_count);
             for (int i = 0; i < blocks_allocated_count; ++i) {
                 if (free_block(fd, &sb_rollback, allocated_blocks[i]) != 0) {
                     // Don't update sb_rollback in loop, free_block writes the SB itself
                      fprintf(stderr, "Rollback warning: Failed to free block %d. Filesystem inconsistent.\n", allocated_blocks[i]);
                 } else {
                     // fprintf(stderr, "Rollback: Freed block %d.\n", allocated_blocks[i]);
                 }
             }
             fprintf(stderr, "Rollback: Finished freeing blocks.\n");
         } else {
              fprintf(stderr, "Rollback warning: Failed to re-read superblock, cannot free allocated blocks.\n");
         }
    }


cleanup:
    if (linux_fd != -1) close(linux_fd);
    if (fd != -1) close(fd);
    free(myfs_fname);
    free(internal_path);
    free(target_name);
    free(data_buf);
    free(allocated_blocks);
    return result;
}


int mycopyFrom_impl(char *myfs_path_spec, const char *linux_fname) {
    char *myfs_fname = NULL;
    char *internal_path = NULL;
    char *source_name = NULL;
    Descriptor source_desc;
    SuperBlock sb;
    int fd = -1, linux_fd = -1;
    char *data_buf = NULL;
    int result = -1; // Default to error

    // 1. Parse paths
    if (parse_path_spec(myfs_path_spec, &myfs_fname, &internal_path) != 0) {
        goto cleanup;
    }

    // 2. Open myfs file and read superblock
    fd = open(myfs_fname, O_RDONLY); // Read-only is sufficient
    if (fd == -1) {
        perror("Error opening myfs file");
        goto cleanup;
    }
    if (read_superblock(fd, &sb) != 0) {
        fprintf(stderr, "Error reading superblock from %s\n", myfs_fname);
        goto cleanup;
    }
    data_buf = malloc(sb.block_size);
     if (!data_buf) { perror("malloc failed for data buffer"); goto cleanup;}


    // 3. Traverse path to find the source file descriptor
    int traverse_status = traverse_path(fd, &sb, internal_path, NULL, &source_name); // Get name first

    if (traverse_status < 0) { // Error during traversal
        fprintf(stderr, "Error traversing path '%s' in %s\n", internal_path, myfs_fname);
        goto cleanup;
    }
     int parent_block;
     if (traverse_status == 0) { // Path was root "/"
         fprintf(stderr, "Error: Cannot copyFrom root directory '/'\n");
         goto cleanup;
     } else {
         parent_block = traverse_status;
     }

     if (!source_name) {
         fprintf(stderr, "Internal error: Failed to get source name from path traversal for '%s'.\n", internal_path);
         goto cleanup;
     }

     // Verify the entry exists in the parent directory and get its descriptor
     if (find_entry_in_dir(fd, &sb, parent_block, source_name, &source_desc, NULL, NULL) != 0) {
         fprintf(stderr, "Error: Source '%s' not found in directory within %s\n", source_name, myfs_path_spec);
         goto cleanup;
     }

    if (source_desc.type != TYPE_FILE) {
        fprintf(stderr, "Error: '%s' (resolved to '%s') is not a file.\n", myfs_path_spec, source_name);
        goto cleanup;
    }

    // 4. Open linux file for writing
    linux_fd = open(linux_fname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (linux_fd == -1) {
        perror("Error opening destination linux file for writing");
        goto cleanup;
    }

    // 5. Copy data block by block
    off_t bytes_to_copy = source_desc.size;
    off_t bytes_copied = 0;
    int current_myfs_block = source_desc.first_block;
    int data_bytes_per_block = sb.block_size - sizeof(int);

     // Handle 0-byte file case explicitly (no blocks to read)
     if (bytes_to_copy == 0) {
          // File was created truncated, nothing more to do.
          result = 0;
          goto cleanup;
     }

    while (bytes_copied < bytes_to_copy && current_myfs_block != 0) {
        if (read_block(fd, current_myfs_block, data_buf, sb.block_size) != 0) {
            fprintf(stderr, "Error reading data block %d during copy from myfs\n", current_myfs_block);
            goto cleanup; // File system might be corrupt, abort
        }

        // Determine how much data to write from this block
        off_t chunk_size = data_bytes_per_block;
        if (bytes_copied + chunk_size > bytes_to_copy) {
            chunk_size = bytes_to_copy - bytes_copied;
             if (chunk_size < 0) chunk_size = 0; // Avoid negative size if somehow overshot
        }

         if(chunk_size > 0) {
            ssize_t bytes_written = write(linux_fd, data_buf, chunk_size);
            if (bytes_written == -1) {
                perror("Error writing to destination linux file");
                goto cleanup; // Error writing to linux fs
            }
            if (bytes_written < chunk_size) {
                fprintf(stderr, "Error: Short write to linux file (disk full?)\n");
                goto cleanup; // Error writing to linux fs
            }
            bytes_copied += bytes_written;
         } else {
             // This block contains no actual data needed for the file size
             // (e.g., file size ends exactly at block boundary, but chain continues)
             // Or error case chunk_size < 0
             break; // Stop reading myfs chain
         }

        // Get next block number if we haven't copied everything
        if (bytes_copied < bytes_to_copy) {
             memcpy(&current_myfs_block, data_buf + sb.block_size - sizeof(int), sizeof(int)); // Fixed: & added
        } else {
            current_myfs_block = 0; // Copied everything, stop.
        }
    }

     if (bytes_copied != bytes_to_copy) {
         fprintf(stderr, "Warning: Copied %lld bytes, but file size in descriptor was %lld. Filesystem potentially corrupt or copy incomplete.\n",
                 (long long)bytes_copied, (long long)source_desc.size);
         // Treat as error because we didn't get expected size
         goto cleanup;
     }
      if (current_myfs_block != 0 && bytes_copied == bytes_to_copy) {
            fprintf(stderr, "Warning: Reached expected file size, but block chain continues (next block %d). Filesystem potentially corrupt.\n", current_myfs_block);
            // Treat as success, but warn
      }


    result = 0; // Success

cleanup:
    if (linux_fd != -1) {
         if(close(linux_fd) == -1) {
              perror("Error closing destination linux file");
              result = -1; // Mark error if close fails
         }
    }
    if (fd != -1) close(fd);
    free(myfs_fname);
    free(internal_path);
    free(source_name);
    free(data_buf);
    // If error occurred during write, the linux file might be partial.
     if (result != 0 && linux_fname) {
          unlink(linux_fname); // Try to remove partial linux file on error
     }
    return result;
}

int myrm_impl(char *myfs_path_spec) {
    char *myfs_fname = NULL;
    char *internal_path = NULL;
    char *target_name = NULL;
    Descriptor target_desc;
    SuperBlock sb;
    int fd = -1;
    char *block_buf = NULL;
    int result = -1; // Default error

    // 1. Parse path
    if (parse_path_spec(myfs_path_spec, &myfs_fname, &internal_path) != 0) {
        goto cleanup;
    }
     if (strcmp(internal_path, "/") == 0) {
         fprintf(stderr, "Error: Cannot remove root directory '/' with myrm.\n");
         goto cleanup;
     }

    // 2. Open FS and read SB
    fd = open(myfs_fname, O_RDWR);
    if (fd == -1) { perror("Error opening myfs file"); goto cleanup; }
    if (read_superblock(fd, &sb) != 0) { fprintf(stderr, "Error reading superblock from %s\n", myfs_fname); goto cleanup; }
    block_buf = malloc(sb.block_size);
    if (!block_buf) {perror("malloc failed for block buffer"); goto cleanup;}

    // 3. Find parent and target descriptor
    int traverse_status = traverse_path(fd, &sb, internal_path, NULL, &target_name);
    if (traverse_status < 0) { fprintf(stderr, "Error traversing path '%s' in %s\n", internal_path, myfs_fname); goto cleanup;}

     int parent_block;
     if (traverse_status == 0) { // Path was root "/" - this should have been caught earlier
          fprintf(stderr, "Internal Error: Traverse path returned root for non-root rm path '%s'\n", internal_path);
          goto cleanup;
     } else {
          parent_block = traverse_status;
     }

     if (!target_name) { fprintf(stderr,"Internal error: no target name from traverse for '%s'\n", internal_path); goto cleanup;}

     // Verify the entry exists in the parent and get its descriptor
     if (find_entry_in_dir(fd, &sb, parent_block, target_name, &target_desc, NULL, NULL) != 0) {
         fprintf(stderr, "Error: File '%s' not found in directory within %s\n", target_name, myfs_path_spec);
         goto cleanup;
     }

     if (target_desc.type != TYPE_FILE) {
         fprintf(stderr, "Error: '%s' (resolved to '%s') is not a file. Use myrmdir for directories.\n", myfs_path_spec, target_name);
         goto cleanup;
     }

    // 4. Free data blocks
    int current_block = target_desc.first_block;
    int next_block;
    int blocks_freed_count = 0;
    while (current_block != 0) {
        // Read block just to get the next pointer before freeing
        if (read_block(fd, current_block, block_buf, sb.block_size) != 0) {
             fprintf(stderr, "Error reading block %d while freeing chain (continuing removal, FS may be inconsistent)\n", current_block);
             // Cannot get next pointer, must stop freeing chain.
             next_block = 0; // Stop chain processing
             // We should probably error out completely here as we can't guarantee freeing all blocks.
             result = -1;
             goto cleanup; // Abort the rm operation
        } else {
            memcpy(&next_block, block_buf + sb.block_size - sizeof(int), sizeof(int));
        }

        // Free the current block (free_block updates and writes SB)
        if (free_block(fd, &sb, current_block) != 0) {
            fprintf(stderr, "Error freeing block %d (continuing removal attempt, FS may be inconsistent)\n", current_block);
            // Proceed, but the free list might be corrupt
        } else {
             blocks_freed_count++;
        }
        current_block = next_block;
    }
     // fprintf(stderr, "Debug: Freed %d data blocks for %s.\n", blocks_freed_count, target_name);


    // 5. Remove entry from parent directory
    // Superblock sb might have changed due to free_block calls, but remove_entry doesn't need SB *content*.
    if (remove_entry_from_dir(fd, &sb, parent_block, target_name) != 0) {
        fprintf(stderr, "Error removing directory entry for '%s' after freeing blocks. FS inconsistent.\n", target_name);
        // Data blocks freed, but entry remains. Critical inconsistency.
        result = -1;
        goto cleanup; // Error out
    }

    result = 0; // Success

cleanup:
     if (fd != -1) close(fd);
     free(myfs_fname);
     free(internal_path);
     free(target_name);
     free(block_buf);
     return result;
}


int mymkdir_impl(char *mydir_path_spec) {
     char *myfs_fname = NULL;
     char *internal_path = NULL;
     char *new_dir_name = NULL;
     SuperBlock sb;
     int fd = -1;
     int new_dir_data_block = -1; // Track allocated block for rollback
     char* block_buf = NULL;
     int result = -1; // Default error

    // 1. Parse path
     if (parse_path_spec(mydir_path_spec, &myfs_fname, &internal_path) != 0) {
         goto cleanup;
     }
      if (strcmp(internal_path, "/") == 0) {
          fprintf(stderr, "Error: Cannot create root directory '/' (it already exists).\n");
          goto cleanup;
      }

     // 2. Open FS and read SB
     fd = open(myfs_fname, O_RDWR);
     if (fd == -1) { perror("Error opening myfs file"); goto cleanup; }
     if (read_superblock(fd, &sb) != 0) { fprintf(stderr, "Error reading superblock from %s\n", myfs_fname); goto cleanup; }

     // 3. Traverse path to find parent and check if target exists
     int traverse_status = traverse_path(fd, &sb, internal_path, NULL, &new_dir_name);
     if (traverse_status < 0) { fprintf(stderr, "Error traversing path '%s' in %s\n", internal_path, myfs_fname); goto cleanup; }

      int parent_block;
      if (traverse_status == 0) { // Parent is root
           parent_block = sb.root_dir_block;
      } else {
           parent_block = traverse_status;
      }

      if (!new_dir_name || strcmp(new_dir_name, "/") == 0) {
           fprintf(stderr,"Internal error: no valid directory name derived from '%s'\n", internal_path);
           goto cleanup;
      }


     // Check if the target name already exists in the parent directory `parent_block`
     Descriptor existing_desc;
     if (find_entry_in_dir(fd, &sb, parent_block, new_dir_name, &existing_desc, NULL, NULL) == 0) {
         fprintf(stderr, "Error: Target '%s' already exists in directory within %s\n", new_dir_name, mydir_path_spec);
         goto cleanup;
     }

    // 4. Check dir name length
     if (strlen(new_dir_name) >= MAX_FILENAME_LEN) {
         fprintf(stderr, "Error: Directory name '%s' is too long (max %d chars).\n", new_dir_name, MAX_FILENAME_LEN-1);
         goto cleanup;
     }

     // 5. Allocate block for the new directory's data
     new_dir_data_block = allocate_block(fd, &sb); // allocate_block updates and writes SB
     if (new_dir_data_block == -1) {
         fprintf(stderr, "Error allocating block for new directory (disk full?)\n");
         goto cleanup; // No rollback needed yet
     }

     // 6. Initialize the new directory block (empty)
     block_buf = calloc(1, sb.block_size); // Zero out, sets next pointer to 0
     if (!block_buf) { perror("calloc failed for dir buffer"); goto rollback; } // Need rollback for allocated block
     if (write_block(fd, new_dir_data_block, block_buf, sb.block_size) != 0) {
          fprintf(stderr, "Error writing new directory block %d\n", new_dir_data_block);
          goto rollback; // Need rollback for allocated block
     }
     free(block_buf); block_buf=NULL; // Free immediately after use

     // 7. Create descriptor for the new directory
     Descriptor new_dir_desc;
     memset(&new_dir_desc, 0, sizeof(Descriptor));
     new_dir_desc.type = TYPE_DIR;
     strncpy(new_dir_desc.name, new_dir_name, MAX_FILENAME_LEN);
     new_dir_desc.name[MAX_FILENAME_LEN - 1] = '\0'; // Ensure null termination
     new_dir_desc.first_block = new_dir_data_block;
     new_dir_desc.size = 0; // Size is 0 for empty directory

     // 8. Add descriptor to parent directory
     // SB may have changed from allocate_block. add_entry_to_dir reads blocks directly.
     if (add_entry_to_dir(fd, &sb, parent_block, &new_dir_desc) != 0) {
         fprintf(stderr, "Error adding directory entry to parent directory\n");
         goto rollback; // Rollback allocated block
     }

     result = 0; // Success
     goto cleanup;

rollback:
     fprintf(stderr, "Error occurred, attempting rollback...\n");
     result = -1; // Ensure error status
     if (new_dir_data_block != -1 && fd != -1) {
          // Need to re-read SB as allocate_block wrote it.
         SuperBlock sb_rollback;
         if (read_superblock(fd, &sb_rollback) == 0) {
              if (free_block(fd, &sb_rollback, new_dir_data_block) != 0) {
                   fprintf(stderr, "Rollback warning: Failed to free allocated block %d. FS inconsistent.\n", new_dir_data_block);
              } else {
                  fprintf(stderr, "Rollback: Freed allocated block %d.\n", new_dir_data_block);
              }
         } else {
              fprintf(stderr, "Rollback warning: Failed read superblock, cannot free block %d.\n", new_dir_data_block);
         }
     }


 cleanup:
      if (fd != -1) close(fd);
      free(myfs_fname);
      free(internal_path);
      free(new_dir_name);
      free(block_buf); // In case of error after allocation but before write
      return result;
}

int myrmdir_impl(char *mydir_path_spec) {
     char *myfs_fname = NULL;
     char *internal_path = NULL;
     char *target_name = NULL;
     Descriptor target_desc;
     SuperBlock sb;
     int fd = -1;
     char* block_buf = NULL; // Only needed if we handle non-empty dir block chains (which we shouldn't)
     int result = -1; // Default error

     // 1. Parse path
     if (parse_path_spec(mydir_path_spec, &myfs_fname, &internal_path) != 0) {
         goto cleanup;
     }
      if (strcmp(internal_path, "/") == 0) {
          fprintf(stderr, "Error: Cannot remove root directory '/'.\n");
          goto cleanup;
      }


     // 2. Open FS and read SB
     fd = open(myfs_fname, O_RDWR);
     if (fd == -1) { perror("Error opening myfs file"); goto cleanup; }
     if (read_superblock(fd, &sb) != 0) { fprintf(stderr, "Error reading superblock from %s\n", myfs_fname); goto cleanup; }

     // 3. Find parent and target descriptor
     int traverse_status = traverse_path(fd, &sb, internal_path, NULL, &target_name);
     if (traverse_status < 0) { fprintf(stderr, "Error traversing path '%s' in %s\n", internal_path, myfs_fname); goto cleanup;}

      int parent_block;
      if (traverse_status == 0) { // Parent is root - this should not happen for rmdir /
          fprintf(stderr, "Internal Error: Traverse path returned root for non-root rmdir path '%s'\n", internal_path);
          goto cleanup;
      } else {
          parent_block = traverse_status;
      }

      if (!target_name || strcmp(target_name, "/") == 0) {
          fprintf(stderr,"Internal error: no valid directory name derived from '%s'\n", internal_path);
          goto cleanup;
      }


     // Verify the entry exists in the parent and get its descriptor
     if (find_entry_in_dir(fd, &sb, parent_block, target_name, &target_desc, NULL, NULL) != 0) {
         fprintf(stderr, "Error: Directory '%s' not found in directory within %s\n", target_name, mydir_path_spec);
         goto cleanup;
     }
      if (target_desc.type != TYPE_DIR) {
          fprintf(stderr, "Error: '%s' (resolved to '%s') is not a directory. Use myrm for files.\n", mydir_path_spec, target_name);
          goto cleanup;
      }

     // 4. Check if directory is empty
     int empty_status = is_directory_empty(fd, &sb, target_desc.first_block);
     if (empty_status == 0) { // Not empty
         fprintf(stderr, "Error: Directory '%s' (resolved to '%s') is not empty.\n", mydir_path_spec, target_name);
         goto cleanup;
     } else if (empty_status == -1) { // Error checking
         fprintf(stderr, "Error checking if directory '%s' is empty.\n", mydir_path_spec);
         goto cleanup;
     }
     // Directory is empty if empty_status == 1

     // 5. Free directory's data block(s)
     // An empty directory created by mymkdir should only have one block.
     // If it somehow had more (corruption?), freeing the chain is complex without rollback.
     // Let's assume it only has the one block allocated at creation.
     int dir_data_block = target_desc.first_block;
     if (dir_data_block != 0) { // Should not be 0 if type is DIR unless FS is corrupt
          // Free the single data block (free_block updates and writes SB)
          if (free_block(fd, &sb, dir_data_block) != 0) {
               fprintf(stderr, "Error freeing directory data block %d for '%s'. FS inconsistent.\n", dir_data_block, target_name);
               // Proceed to try removing entry anyway? Or abort? Abort is safer.
               goto cleanup;
          }
     } else {
           fprintf(stderr, "Warning: Directory '%s' descriptor points to block 0. Skipping block free.\n", target_name);
     }


     // 6. Remove entry from parent directory
     // SB might have changed. remove_entry doesn't need SB content.
     if (remove_entry_from_dir(fd, &sb, parent_block, target_name) != 0) {
         fprintf(stderr, "Error removing directory entry for '%s' after freeing block. FS inconsistent.\n", target_name);
         // Block freed, but entry remains. Critical inconsistency.
         // Should try to re-allocate and link block? Very hard. Error out.
         goto cleanup; // Error out
     }

     result = 0; // Success

 cleanup:
      if (fd != -1) close(fd);
      free(myfs_fname);
      free(internal_path);
      free(target_name);
      free(block_buf); // Just in case it was allocated
      return result;
}


// --- Library Functions (Not Commands) ---

// Reads the *logical* block number `logical_block_no` (0-indexed) of the myfile into user buffer `buf`.
// Assumes buf is large enough (sb.block_size).
// Returns 0 on success, -1 on error.
int myreadBlock_impl(char *myfname_spec, char *buf, int logical_block_no) {
    char *myfs_fname = NULL;
    char *internal_path = NULL;
    char *target_name = NULL;
    Descriptor target_desc;
    SuperBlock sb;
    int fd = -1;
    char* temp_buf = NULL; // For reading intermediate blocks to find next pointer
    int result = -1;

    if (logical_block_no < 0) {
        fprintf(stderr, "Error: Logical block number cannot be negative.\n");
        goto cleanup;
    }

    if (parse_path_spec(myfname_spec, &myfs_fname, &internal_path) != 0) goto cleanup;

    fd = open(myfs_fname, O_RDONLY);
    if (fd == -1) { perror("Error opening myfs file"); goto cleanup; }
    if (read_superblock(fd, &sb) != 0) { fprintf(stderr, "Error reading superblock from %s\n", myfs_fname); goto cleanup; }

    // Find the file descriptor
    int traverse_status = traverse_path(fd, &sb, internal_path, NULL, &target_name);
     if (traverse_status < 0) { fprintf(stderr, "Error traversing path '%s' in %s\n", internal_path, myfs_fname); goto cleanup;}

      int parent_block;
      if (traverse_status == 0) { // Path was root "/"
          fprintf(stderr, "Error: Cannot readBlock on root directory '/'\n");
          goto cleanup;
      } else {
          parent_block = traverse_status;
      }

      if (!target_name || strcmp(target_name,"/") == 0) {
           fprintf(stderr,"Internal error: no valid file name derived from '%s'\n", internal_path);
           goto cleanup;
      }

     // Verify the entry exists in the parent and get its descriptor
     if (find_entry_in_dir(fd, &sb, parent_block, target_name, &target_desc, NULL, NULL) != 0) {
         fprintf(stderr, "Error: File '%s' not found in directory within %s\n", target_name, myfname_spec);
         goto cleanup;
     }
     if (target_desc.type != TYPE_FILE) {
         fprintf(stderr, "Error: '%s' (resolved to '%s') is not a file.\n", myfname_spec, target_name);
         goto cleanup;
     }

    // Calculate max logical block number possible for the file size
    int data_bytes_per_block = sb.block_size > (int)sizeof(int) ? sb.block_size - sizeof(int) : 0;
    int max_logical_block = -1; // For 0-byte file
    if (target_desc.size > 0 && data_bytes_per_block > 0) {
         max_logical_block = (target_desc.size - 1) / data_bytes_per_block;
    } else if (target_desc.size == 0) {
         max_logical_block = -1; // No data blocks exist for 0-byte file
    }


     // Check if logical block number is valid
     if (logical_block_no < 0 || // Already checked, but good practice
         (target_desc.size == 0 && logical_block_no > 0) || // Cannot read block > 0 of 0-byte file
         (target_desc.size > 0 && (logical_block_no > max_logical_block || data_bytes_per_block == 0)) // Check against calculated max block
        )
     {
         fprintf(stderr, "Error: Logical block number %d is out of range for file '%s' (size %d, max block %d).\n",
                 logical_block_no, target_name, target_desc.size, max_logical_block);
         goto cleanup;
     }


    // Handle 0-byte file read block 0 explicitly
     if (target_desc.size == 0 && logical_block_no == 0) {
          // Reading logical block 0 of an empty file. Return an empty block.
          // Note: The file descriptor points to first_block=0 in this case.
          memset(buf, 0, sb.block_size);
          result = 0; // Success
          goto cleanup;
     }


    // Traverse the block chain to find the physical block number
    int current_physical_block = target_desc.first_block;
    temp_buf = malloc(sb.block_size);
    if (!temp_buf) { perror("malloc failed for temp buffer"); goto cleanup; }

    for (int i = 0; i < logical_block_no; ++i) {
        if (current_physical_block == 0) {
            // This should not happen if range check above is correct, indicates FS corruption
            fprintf(stderr, "Error: Logical block number %d is out of range (chain ended unexpectedly at block %d for file '%s'). FS Corrupt.\n",
                    logical_block_no, i, target_name);
            goto cleanup;
        }
        if (read_block(fd, current_physical_block, temp_buf, sb.block_size) != 0) {
            fprintf(stderr, "Error reading block %d while traversing file chain for '%s'.\n", current_physical_block, target_name);
            goto cleanup;
        }
        memcpy(&current_physical_block, temp_buf + sb.block_size - sizeof(int), sizeof(int)); // Fixed: & added
    }

    // We have the target physical block number in current_physical_block
    if (current_physical_block == 0) {
        // Should not happen if range check is correct, unless logical_block_no was 0 for a >0 size file?
        // Or if chain ended exactly at the block we needed.
         fprintf(stderr, "Error: Target physical block is 0 after traversing chain for logical block %d of file '%s'. FS Corrupt.\n",
                 logical_block_no, target_name);
        goto cleanup;
    }

    // Read the target physical block into the user's buffer
    if (read_block(fd, current_physical_block, buf, sb.block_size) != 0) {
        fprintf(stderr, "Error reading target physical block %d for file '%s'.\n", current_physical_block, target_name);
        goto cleanup;
    }

    result = 0; // Success

cleanup:
    if (fd != -1) close(fd);
    free(myfs_fname);
    free(internal_path);
    free(target_name);
    free(temp_buf);
    return result;
}


// Stores the 21-byte descriptor for the myfile/mydirectory in the user buffer `buf`.
// Returns 0 on success, -1 on error.
int mystat_impl(char *myname_spec, char *buf) {
    char *myfs_fname = NULL;
    char *internal_path = NULL;
    char *target_name = NULL;
    Descriptor target_desc;
    SuperBlock sb;
    int fd = -1;
    int result = -1;

    // Ensure buffer is not NULL
    if (!buf) {
         fprintf(stderr, "Error (mystat): Output buffer cannot be NULL.\n");
         goto cleanup; // Skip parse etc. if buffer invalid
    }
    memset(buf, 0, DESCRIPTOR_SIZE); // Clear user buffer initially

    if (parse_path_spec(myname_spec, &myfs_fname, &internal_path) != 0) goto cleanup;

    fd = open(myfs_fname, O_RDONLY);
    if (fd == -1) { perror("Error opening myfs file"); goto cleanup; }
    if (read_superblock(fd, &sb) != 0) { fprintf(stderr, "Error reading superblock from %s\n", myfs_fname); goto cleanup; }

    // Find the descriptor
    int traverse_status = traverse_path(fd, &sb, internal_path, NULL, &target_name);
     if (traverse_status < 0) {
          fprintf(stderr, "Error traversing path '%s' in %s\n", internal_path, myfs_fname);
          goto cleanup;
     }

     if (traverse_status == 0) { // Path was root "/"
          // Special case: Stat root. Create a pseudo-descriptor.
          target_desc.type = TYPE_DIR;
          strncpy(target_desc.name, "/", MAX_FILENAME_LEN);
          target_desc.name[MAX_FILENAME_LEN -1] = '\0';
          target_desc.first_block = sb.root_dir_block;
          target_desc.size = 0;
     } else {
          int parent_block = traverse_status;
          // Verify the entry exists in the parent and get its descriptor
          if (!target_name || strcmp(target_name, "/") == 0) {
               fprintf(stderr,"Internal error: no valid name from traverse for stat '%s'\n", internal_path);
               goto cleanup;
          }
          if (find_entry_in_dir(fd, &sb, parent_block, target_name, &target_desc, NULL, NULL) != 0) {
               fprintf(stderr, "Error: File or directory '%s' not found in directory within %s\n", target_name, myname_spec);
               goto cleanup;
          }
     }

    memcpy(buf, &target_desc, DESCRIPTOR_SIZE);
    result = 0;

cleanup:
    if (fd != -1) close(fd);
    free(myfs_fname);
    free(internal_path);
    free(target_name);
    return result;
}
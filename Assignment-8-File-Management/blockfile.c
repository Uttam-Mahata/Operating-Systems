/* Following is a sample program to demonstrate how fixed size records (that is, structures in C) can be written at arbitrary positions in a file or can be read from arbitrary positions of a file. */
i
#include <stdio.h>
/* following header files are included as suggested by manuals for
   open(2), lseek(2), write(2), close(2)
*/
#include <sys/types.h> /* for open(2) lseek(2) */
#include <sys/stat.h> /* for open(2) */
#include <fcntl.h> /* for open(2) */
#include <unistd.h> /* for lseek(2), write(2), close(2)*/
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memset() */
#define BLOCK_SIZE 4096 /*SIZE of each block in the file*/

typedef struct block_record {
	int n; /*Number of records in the block*/
	int size; /*Size of each record in the block*/
	int ubn; /*Number of used block*/
	int fbn; /*Number of free block*/
	int ub; /*Used block bit pattern*/
} BlockRecord;

/*Function Prototype*/

// int init_File_dd(const char *fname, int bsize, int bno); /* creates a file (if it does not exist) called fname of appropriate size (4096 + bsize*bno Bytes) in your folder. The function initializes the first 4096 Byes putting proper values for n, s, ubn, fbn, and ub. If for some reason this functions fails, it returns -1. Otherwise it returns 0.
// */
// int get_freeblock(const char *fname); /* reads the 1st 4096 Bytes (containing n, s, ubn, fbn, ub) from the file fname  and finds and returns the block-number of the 1st free block (starting from the 0th bloc) in that file. This function sets the corresponding  bit in ub and fills up the free block with 1's.  ubn and fbn too  are modified appropriately.  On failure this function returns -1. Please  note that all the modifications done by this function have to be written back to the file fname.
// */
// int free_block(const char *fname, int bno);/* reads the 1st 4096 Bytes (containing n, s, ubn, fbn, ub) from the file fname  and frees the block bno, that is,  resets the bit corresponding to the block bno in ub. This function  fills up the  block bno with 0's.  ubn and fbn too  are modified appropriately.  On success  this function returns 1. It returns 0 otherwise.  Please note that all the modifications done by this function have to be written back to the file fname.


// int check_fs(const char *fname);/*
// checks the integrity of the file fname with respect to n, s, ubn, hbn, ub and the contents of the blocks. In case of any inconsistency (say, ubn+fbn ≠  n, ub does not contain ubn number of 1's, etc.), this function returns 1. It returns 0 otherwise.
// 				   */
int main(int argc, char *argv[]) {
    int ifd, ofd;
    BlockRecord br;
    int i, j;
    int bno;
    int bsize;

    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;

        ofd = open("ioput", O_CREAT|O_WRONLY, 0700);
        if (ofd == -1 ) { /* when open() fails it returns -1. */
                fprintf(stderr, "Cannot open file for writing: ");
        }
    

}


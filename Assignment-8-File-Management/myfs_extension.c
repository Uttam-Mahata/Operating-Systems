/* Myfile/Myfolder needs multiple block. These blocks will be maintained as a chain,
 that is, the last 4 bytes of a block would contain the block number of the next block of myfile
(myfolder). myfile descriptor now have one additional field (4 byte), block number of the 1st block ofthe myfile (myfolder). 
The data part of a myfolder will be collection of 21 byte descriptors of the
 myfiles or submyfolders that the myfolder contains.*/

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 
 #define SUPERBLOCK_SIZE 4096
 #define SIZE 12
 
 typedef struct superBlock{
 int block_size;
 int bn; /*Number of blocks in dd1*/
 int ffbn; /* Block number of 1st free block*/
 int rfbn; /*Block number of 1st block of the root folder of myfs*/
 }superBlock;
 
 typedef struct myDescriptor {
 char byte_type[2]; /*1: for myfile, 2: for myfolder*/
 char name[SIZE];
 int bn; /*Block Number of 1st block of myfile/myfolder*/
 int size; /*Size of myfile/myfolder*/
 }myDescriptor;

void initMyfs(char *filename, int bno, int block_size) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Error creating filesystem");
        return;
    }
    superBlock sb;
    sb.block_size = block_size;
    sb.bn = bno;
    sb.ffbn = 1;
    sb.rfbn = 1;
    if (write(fd, &sb, sizeof(superBlock)) != sizeof(superBlock)) {
        perror("Error writing superblock");
        close(fd);
        return;
    }
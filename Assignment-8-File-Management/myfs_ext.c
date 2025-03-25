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

//Initialize block chaining

int initializeBlockChaining(int fd, int block_num) {
    char buffer[4];
    memset(buffer, 0, sizeof(buffer));
    if (lseek(fd, block_num * sizeof(buffer), SEEK_SET) == -1) {
        perror("Error seeking to block");
        return -1;
    }
    if (write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
        perror("Error writing block");
        return -1;
    }
    return 0;
}



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

int createFile(char *filename, char *file_name) {
    int fd = open(filename,O_RDWR);
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

void mymkfs(char *filename, int bno, int block_size) {
    if (createFileSystem(filename, bno, block_size) == 0) {
        printf("Filesystem created successfully\n");
    } else {
        printf("Error creating filesystem\n");
    }
}
void mymount(char *filename) {
    if (mountFileSystem(filename) == 0) {
        printf("Filesystem mounted successfully\n");
    } else {
        printf("Error mounting filesystem\n");
    }
}
void mycreatefile(char *filename, char *file_name) {
    if (createFile(filename, file_name) == 0) {
        printf("File created successfully\n");
    } else {
        printf("Error creating file\n");
    }
}

void mycreatefolder(char *filename, char *folder_name) {
    if (createFolder(filename, folder_name) == 0) {
        printf("Folder created successfully\n");
    } else {
        printf("Error creating folder\n");
    }
}

void mycopyto(char *filename, char *linux_file) {
    int fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("Error opening filesystem");
        return;
    }
}



int main(int argc, char *argv[]) {

    //mymkfs dd1 bno, block size
    //mycopyto
    //mycopyfrom 


    


}



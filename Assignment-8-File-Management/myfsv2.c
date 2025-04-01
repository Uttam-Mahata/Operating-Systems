/* Filename: myfsv2.c 
myfile (myfolder) needs multiple blocks. These blocks will be maintained as a chain,
that is, the last 4 bytes of a block would contain the block number of the next block of myfile
(myfolder). myfile descriptor now have one additional field (4 byte), block number of the 1st block of 
the myfile (myfolder). The data part of a myfolder will be collection of 21 byte descriptors of the
myfiles or submyfolders that the myfolder contains.*/

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
   int mymkfs(const char *fname, int block_size, int num_blocks);
   int mycopyTo(const char *fname, char *myfname); 
   int mycopyFrom(char *myfname, const char *fname);
   int myrm(const char *path);
   int mymkdir(char *mydirname);
   int myrmdir(const char *path);
   int myreadSBlocks(int fd, char *sbuf);
   int mywriteSBlocks(int fd, char *sbuf);
   int myreadBlock(char *myfname,  char *buf, int block_no) ;
   int mywriteBlock(char *myfname, char *buf, int block_no);
   int mystat(char *myname,  char *buf);
   int findFreeBlock(int fd);
   int allocateBlock(int fd, int *block_num);
   void freeBlock(int fd, int block_num);
   int freeChain(int fd, int first_block);
   int findFile(int fd, const char *path, int *parent_block, int *file_offset, int *file_block);
   int createFileDescriptor(int fd, const char *filename, char type, int first_block, int size, int parent_block);
   char** parseFilePath(const char *path, int *count);
   void freePathComponents(char **components, int count);


   
   /* Initialize a new filesystem */
   int mymkfs(const char *fname, int block_size, int no_of_blocks) {
    int fd;
int i;
int flag;

fd = open(fname, O_CREAT | O_WRONLY, S_IRWXU);
    if (fd == -1) {
        fprintf(stderr,"%s: ", fname);
        perror("File cannot be opened for writing");
        return (-1);
    }
    /* initialize buf */
    char buf[block_size];
    memset(buf, 0, block_size); /* Initialize buf with zeros */
    for (i=0; i < no_of_blocks + 8; i++) {
        flag = write(fd, buf, block_size);
        if(flag == -1) {
            fprintf(stderr,"%s: ", fname);
            perror("File write failed!");
            return (-1);
        }
    }
    close(fd);
    return (0);
}


   int mycopyTo(const char *fname, char *myfname) {
         /* myfile mfname to be copied to linux file fname*/
         /* mfname will be of the from <myfile name>@<myfs file name> */
         int fdFrom;
         int i;
         int flag;       
         char *myfsname;
         char *myfilename;
         struct stat sb;
    
         myfilename = myfname;
         myfsname = strchr(myfname, '@');
         if (myfsname != NULL) {
              *myfsname = '\0';
              myfsname++;
         } else {
              fprintf(stderr, "%s should be of the form <myfile name>@<myfs file name>\n", myfname);
              return(-1);
         }
    
         fdFrom = open(myfsname, O_RDWR);
         if (fdFrom == -1) {
              fprintf(stderr,"%s: ", myfsname);
              perror("File cannot be opened for writing");
              return (-1);
         }
    
         /* get the size of the linux file */
         if (stat(fname, &sb) == -1) {
              fprintf(stderr,"%s: ", fname);
              perror("File cannot be stat'ed");
              close(fdFrom);
              return (-1);
         }
    
         /* check whether fname exists in myfs */
         int hole = -1;
         for (i = 0; i < NOFILES; i++) { /* check all the myfile names in 16 byte myfile descriptors.*/
              if (sbuf[i*16] == 0) {
                hole = i;
              }
              if (strncmp(fname, &(sbuf[i*16]), FNLEN) == 0) {
                break;
              }
         }
    
         if (i >= NOFILES) {
              strcpy(&(sbuf[hole*16]), fname); /* copy the fname in the myfile descriptor */
              *((int *)&(sbuf[hole*16 + 12])) = (int)(sb.st_size); /* copy the fname size in the myfile descriptor */
    
         } else {
              /** Error: File exists */
              fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, myfname);
              fprintf(stderr, "File already exists!\n");
              return (-1);
         }
    
         flag = read(fdFrom, buf, BS);
         if(flag == -1) { /* read() failed */
              fprintf(stderr,"%s: ", fname);
              perror("File read failed!");
              close(fdFrom);
              return (-1);
         }
         //int mywriteBlock(int fd, int bno, char *buf)
         flag = mywriteBlock(fdFrom, 8+hole, buf);
         if (flag == -1) {
              fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, myfname);
              fprintf(stderr, "mywriteBlock() failed!\n");
              close(fdFrom);
              return (-1);
         }
         //int mywriteSBlocks(int fd, char *sbuf)
            flag = mywriteSBlocks(fdFrom, sbuf);
            if (flag == -1) {
                fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, myfname);
                fprintf(stderr, "mywriteSBlocks() failed!\n");
                close(fdFrom);
                return (-1);

            }

            close(fdFrom);

            return (0);

   }


   int mycopyFrom(char *myfname, const char *fname) {
            /* myfile mfname to be copied to linux file fname*/
            /* mfname will be of the from <myfile name>@<myfs file name> */
            int fd;
            int fdFrom;
            int i;
            int flag;       
            char *myfsname;
            char *myfilename;
            struct stat sb;
        
            myfilename = myfname;
            myfsname = strchr(myfname, '@');
            if (myfsname != NULL) {
                *myfsname = '\0';
                myfsname++;
            } else {
                fprintf(stderr, "%s should be of the form <myfile name>@<myfs file name>\n", myfname);
                return(-1);
            }
        
            fdFrom = open(myfsname, O_RDWR);
            if (fdFrom == -1) {
                fprintf(stderr,"%s: ", myfsname);
                perror("File cannot be opened for writing");
                return (-1);
            }
        
            /* check whether myfilename exists in myfs at myfsname */
            for (i = 0; i < BNO; i++) { /* check all the myfile names in 16 byte myfile descriptors.*/
                if (strncmp(myfilename, &(sbuf[i*16]), FNLEN) == 0) { /* myfile found */
                    break;
                }
            }
        
            if (i >= NOFILES) { /* myfile not found */
                /** Error: File not found at myfs */
                fprintf(stderr, "File %s cannot be found in  myfs on %s!\n", myfilename, myfsname);
                return (-1);
            }
        
            /* Read myfile size from the descriptor */
            int myfilesize = *((int *)&(sbuf[i*16 + 12]));
        
            /* Read myfile data */ 
                flag = myreadBlock(fdFrom, 8+i, buf);
                if (flag == -1) {
                    fprintf(stderr, "File %s cannot be read from myfs on %s!\n", myfilename, myfsname);
                    fprintf(stderr, "myreadBlock() failed!\n");
                    close(fdFrom);
                    return (-1);
                }
        
                flag = write(fd, buf, myfilesize);
                if(flag == -1) { /* write() failed */
                    fprintf(stderr,"%s: ", fname);
                    perror("File write failed!");
                    close(fd);
                    return (-1);
                }

                close(fd);
                close(fdFrom);
                return (0);


   }


   int myrm(const char *path) {
         /* myfile mfname to be removed */
         /* mfname will be of the from <myfile name>@<myfs file name> */
         int fdFrom;
         int i;
         int flag;       
         char *myfsname;
         char *myfilename;
     
         myfilename = path;
         myfsname = strchr(path, '@');
         if (myfsname != NULL) {
              *myfsname = '\0';
              myfsname++;
         } else {
              fprintf(stderr, "%s should be of the form <myfile name>@<myfs file name>\n", path);
              return(-1);
         }
     
         fdFrom = open(myfsname, O_RDWR);
         if (fdFrom == -1) {
              fprintf(stderr,"%s: ", myfsname);
              perror("File cannot be opened for writing");
              return (-1);
         }
     
         /* initialize sbuf */
         flag = myreadSBlocks(fdFrom, sbuf);
     
         /* check whether myfilename exists in myfs at myfsname */
         for (i = 0; i < BNO; i++) { /* check all the myfile names in 16 byte myfile descriptors.*/
              if (strncmp(myfilename, &(sbuf[i*16]), FNLEN) == 0) { /* myfile found */
                break;
              }
         }
     
         if (i >= NOFILES) { /* myfile not found */
              /** Error: File not found at myfs */
              fprintf(stderr, "File %s cannot be found in  myfs on %s!\n", myfilename, myfsname);
              return (-1);
         }
     
         /* delete the file in the myfile descriptor */
         sbuf[i*16] = '\0';
         *((int *)&(sbuf[i*16 + 12])) = 0; /* make myfilename size in the myfile descriptor to be 0. optional */
     
     
          //int mywriteSBlocks(int fd, char *sbuf) 
          flag = mywriteSBlocks(fdFrom, sbuf);
          if (flag == -1) {
                fprintf(stderr, "File %s cannot be removed from myfs on %s!\n", myfilename, myfsname);
                fprintf(stderr, "mywriteSBlocks() failed!\n");
                close(fdFrom);
                return (-1);

          }

            close(fdFrom);
            return (0);


   }
   int mymkdir(char *mydirname) {

            /* myfolder mfname to be created */
            /* mfname will be of the from <myfolder name>@<myfs file name> */
            int fdFrom;
            int i;
            int flag;       
            char *myfsname;
            char *myfilename;
        
            myfilename = mydirname;
            myfsname = strchr(mydirname, '@');
            if (myfsname != NULL) {
                *myfsname = '\0';
                myfsname++;
            } else {
                fprintf(stderr, "%s should be of the form <myfolder name>@<myfs file name>\n", mydirname);
                return(-1);
            }
        
            fdFrom = open(myfsname, O_RDWR);
            if (fdFrom == -1) {
                fprintf(stderr,"%s: ", myfsname);
                perror("File cannot be opened for writing");
                return (-1);
            }
        
            /* initialize sbuf */
            flag = myreadSBlocks(fdFrom, sbuf);
        
            /* check whether myfilename exists in myfs at myfsname */
            for (i = 0; i < BNO; i++) { /* check all the myfile names in 16 byte myfile descriptors.*/
                if (strncmp(myfilename, &(sbuf[i*16]), FNLEN) == 0) { /* myfile found */
                    break;
                }
            }
        
            if (i >= NOFILES) { /* myfile not found */
                strcpy(&(sbuf[i*16]), mydirname); /* copy the fname in the myfile descriptor */
                *((int *)&(sbuf[i*16 + 12])) = 0; /* copy the fname size in the myfile descriptor */
        
            } else {
                /** Error: File exists */
                fprintf(stderr, "File %s cannot be created in myfs on %s!\n", mydirname, mydirname);
                fprintf(stderr, "File already exists!\n");
                return (-1);
            }
        
            //int mywriteSBlocks(int fd, char *sbuf) 
            flag = mywriteSBlocks(fdFrom, sbuf);
            if (flag == -1) {
                    fprintf(stderr, "File %s cannot be created in myfs on %s!\n", mydirname, mydirname);
                    fprintf(stderr, "mywriteSBlocks() failed!\n");
                    close(fdFrom);
                    return (-1);
            }

            close(fdFrom);
            return (0);

   }
   int myrmdir(const char *path) {
            /* myfolder mfname to be removed */
            /* mfname will be of the from <myfolder name>@<myfs file name> */
            int fdFrom;
            int i;
            int flag;       
            char *myfsname;
            char *myfilename;
        
            myfilename = path;
            myfsname = strchr(path, '@');
            if (myfsname != NULL) {
                *myfsname = '\0';
                myfsname++;
            } else {
                fprintf(stderr, "%s should be of the form <myfolder name>@<myfs file name>\n", path);
                return(-1);
            }
        
            fdFrom = open(myfsname, O_RDWR);
            if (fdFrom == -1) {
                fprintf(stderr,"%s: ", myfsname);
                perror("File cannot be opened for writing");
                return (-1);
            }
        
            /* initialize sbuf */
            flag = myreadSBlocks(fdFrom, sbuf);
        
            /* check whether myfilename exists in myfs at myfsname */
            for (i = 0; i < BNO; i++) { /* check all the myfile names in 16 byte myfile descriptors.*/
                if (strncmp(myfilename, &(sbuf[i*16]), FNLEN) == 0) { /* myfile found */
                    break;
                }
            }
        
            if (i >= NOFILES) { /* myfile not found */
                /** Error: File not found at myfs */
                fprintf(stderr, "File %s cannot be found in  myfs on %s!\n", myfilename, myfsname);
                return (-1);
            }
        
            /* delete the file in the myfile descriptor */
            sbuf[i*16] = '\0';
            *((int *)&(sbuf[i*16 + 12])) = 0; /* make myfilename size in the myfile descriptor to be 0. optional */
        
        
            //int mywriteSBlocks(int fd, char *sbuf) 
            flag = mywriteSBlocks(fdFrom, sbuf);
            if (flag == -1) {
                    fprintf(stderr, "File %s cannot be removed from myfs on %s!\n", myfilename, myfsname);
                    fprintf(stderr, "mywriteSBlocks() failed!\n");
                    close(fdFrom);
                    return (-1);
            }
            
            close(fdFrom);
            return (0);

   }
   int myreadSBlocks(int fd, char *sbuf) {
         int i;
         int flag;
         flag = lseek(fd, 0, SEEK_SET); /* Going to the beginning of the file */
              if (flag == -1) {
                     perror("lseek() at myreadSBlocks() fails: ");
                     return (flag);
              }
         flag = 0;
         for (i = 0; i < 8 && flag != -1; i++) {
              flag = myreadBlock(fd, &(sbuf[i*BS]), i);
         }
         return (flag); 

   }
   int mywriteSBlocks(int fd, char *sbuf) {
            int i;
            int flag;
            flag = lseek(fd, 0, SEEK_SET); /* Going to the beginning of the file */
                if (flag == -1) {
                        perror("lseek() at mywriteSBlocks() fails: ");
                        return (-1);
                }
            flag = 0;
            for (i = 0; i < 8 && flag != -1; i++) {
                flag = mywriteBlock(fd, &(sbuf[i*BS]), i);
            }
            return (flag);

   }
   int myreadBlock(char *myfname,  char *buf, int block_no)  {
            int fd;
            int flag;
            fd = open(myfname, O_RDWR);
            if (fd == -1) {
                fprintf(stderr,"%s: ", myfname);
                perror("File cannot be opened for writing");
                return (-1);
            }
            flag = lseek(fd, block_no*BS, SEEK_SET); /* Going to the beginning of the file */
                if (flag == -1) {
                        perror("lseek() at myreadBlock() fails: ");
                        return (flag);
                }
            flag = read(fd, buf, BS);
            if(flag == -1) { /* read() failed */
                fprintf(stderr,"%s: ", myfname);
                perror("File read failed!");
                close(fd);
                return (-1);
            }
            close(fd);
            return (0);

   }
   int mywriteBlock(char *myfname, char *buf, int block_no) {

            int fd;
            int flag;
            fd = open(myfname, O_RDWR);
            if (fd == -1) {
                fprintf(stderr,"%s: ", myfname);
                perror("File cannot be opened for writing");
                return (-1);
            }
            flag = lseek(fd, block_no*BS, SEEK_SET); /* Going to the beginning of the file */
                if (flag == -1) {
                        perror("lseek() at mywriteBlock() fails: ");
                        return (flag);
                }
            flag = write(fd, buf, BS);
            if(flag == -1) { /* read() failed */
                fprintf(stderr,"%s: ", myfname);
                perror("File write failed!");
                close(fd);
                return (-1);
            }
            close(fd);
            return (0);

   }
   int mystat(char *myname,  char *buf) {

            /* myfile mfname to be copied to linux file fname*/
            /* mfname will be of the from <myfile name>@<myfs file name> */
            int fdFrom;
            int i;
            int flag;       
            char *myfsname;
            char *myfilename;
        
            myfilename = myname;
            myfsname = strchr(myname, '@');
            if (myfsname != NULL) {
                *myfsname = '\0';
                myfsname++;
            } else {
                fprintf(stderr, "%s should be of the form <myfile name>@<myfs file name>\n", myname);
                return(-1);
            }
        
            fdFrom = open(myfsname, O_RDWR);
            if (fdFrom == -1) {
                fprintf(stderr,"%s: ", myfsname);
                perror("File cannot be opened for writing");
                return (-1);
            }
        
            /* initialize sbuf */
            flag = myreadSBlocks(fdFrom, sbuf);
        
            /* check whether myfilename exists in myfs at myfsname */
            for (i = 0; i < BNO; i++) { /* check all the myfile names in 16 byte myfile descriptors.*/
                if (strncmp(myfilename, &(sbuf[i*16]), FNLEN) == 0) { /* myfile found */
                    break;
                }
            }
        
            if (i >= NOFILES) { /* myfile not found */
                /** Error: File not found at myfs */
                fprintf(stderr, "File %s cannot be found in  myfs on %s!\n", myfilename, myfsname);
                return (-1);
            }
        
            //int mystat(char *myname,  char *buf)
            memcpy(buf, &(sbuf[i*16]), DESCRIPTOR_SIZE); // Copy the descriptor to buf
            close(fdFrom);
            return (0);



   }
   int findFreeBlock(int fd) {
            int i;
            char buf[BS];
            int flag;
            flag = lseek(fd, 0, SEEK_SET); /* Going to the beginning of the file */
                if (flag == -1) {
                        perror("lseek() at findFreeBlock() fails: ");
                        return (flag);
                }
            flag = read(fd, buf, BS);
            if(flag == -1) { /* read() failed */
                fprintf(stderr,"%s: ", "myfs");
                perror("File read failed!");
                close(fd);
                return (-1);
            }
            for (i = 0; i < BNO; i++) {
                if (buf[i] == 0) {
                    return i;
                }
            }
            return -1;

   }
   int allocateBlock(int fd, int *block_num) {

   }
   void freeBlock(int fd, int block_num) {

   }
   int freeChain(int fd, int first_block) {

   }
   int findFile(int fd, const char *path, int *parent_block, int *file_offset, int *file_block) {

   }
   int createFileDescriptor(int fd, const char *filename, char type, int first_block, int size, int parent_block) {

   }
   char** parseFilePath(const char *path, int *count) {

   }
   void freePathComponents(char **components, int count) {

   }


   
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
           if(argc != 4) {
               fprintf(stderr, "Usage: %s <linux file>\n", argv[0]);
               exit(1);
           }
           flag = mymkfs(argv[1], atoi(argv[2]), atoi(argv[3]));
   
           if(flag != 0) {
               fprintf(stderr, "%s Failed!\n", argv[0]);
           }
   
       } else if (strcmp(basename, "mycopyTo") == 0) {
           if (argc < 3 || argc > 4) {
               fprintf(stderr, "Usage: %s <linux file> <storage file> [destination folder path]\n", argv[0]);
               exit(1);
           }
           
           
           flag = mycopyTo(argv[1], argv[2]);
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
/* Following is a sample program to demonstrate how fixed size records (that is, structures in C) can be written at arbitrary positions in a file or can be read from arbitrary positions of a file. */

#include <stdio.h>
/* following header files are included as suggested by manuals for
   open(2), lseek(2), write(2), close(2)
*/
#include <sys/types.h> /* for open(2) lseek(2) */
#include <sys/stat.h> /* for open(2) */
#include <fcntl.h> /* for open(2) */
#include <unistd.h> /* for lseek(2), write(2), close(2)*/

/* Here we are maintaining a Book ("Register") of "Records". Please go through the code given below. It should be self-explanatory */

#define MAXR 10 /* Maximum number of records supported by the Register */

/* Defining a type for records */
typedef struct rec {
        char field1[20];
        int field2;
} Record;

/* Defining a type for register which will have a number of records */
typedef struct reg {
        int count; /* Number  of records actually kept in the register */
        Record rs[MAXR]; /* Records */
} Register;

int getCount(Register reg); /* Returns the number of records available in the register reg*/
int putNthRec(int fd, Record r, int n); /* Write the record r at the n-th record position in the file with descriptor */
int getNthRec(int fd, Record *r, int n); /* Read the n-th record from the file having descriptor fd into record pointed to by r */
void displayRec(Record r); /* Displays the record r */
void displayReg(Register reg); /* Displays the records of reg */

int main() {
        int ifd, ofd;
        Register reg1 = {5, {{"ab", 9}, {"bc", 10}, {"cd", 11}, {"de", 12}, {"ef", 13}}}; /* reg1 is a register which is populated here itself */
        Register reg2;

        int regsize1; /* stores number of records in  reg1 */
        int recsize1; /* stores the size of each record in reg1 */

        int regsize2; /* stores number of records in  reg2 */
        int recsize2; /* stores the size of each record in reg2 */


        int i;

        regsize1 = getCount(reg1);
        recsize1 = sizeof(Record);


        /* writing all the records in reg1 into a file "ioput" */

        /* "opening" the file using the system call "open() */
        //int open(const char *pathname, int flags, mode_t mode);
        ofd = open("ioput", O_CREAT|O_WRONLY, 0700);
        if (ofd == -1 ) { /* when open() fails it returns -1. */
                fprintf(stderr, "Cannot open file for writing: ");
        }

        for (i=0; i < regsize1; i++) {
                int status;
                status =  putNthRec(ofd, reg1.rs[i], i);
                if(status == 0) {
                        fprintf(stderr, "Cannot write %dth record!\n", i+1);
                }
        }
        close (ofd);

        printf("Reg1\n");
        displayReg(reg1);

        /* reading altername records from the file "ioput" into register reg2 */

        /* "opening" the file using the system call "open() */
        //int open(const char *pathname, int flags, mode_t mode);
        ifd = open("ioput", O_RDONLY);
        if (ifd == -1 ) { /* When open() fails it returns -1 */
                fprintf(stderr, "Cannot open file in for reading: ");
        }

        reg2.count = 0;
        for (i=0; i < regsize1; i = i + 2) {
                int status;
                status =  getNthRec(ifd, &(reg2.rs[reg2.count]), i);
                if(status == 0) {

                fprintf(stderr, "Cannot read %dth record!\n", i+1);
                } else  {
                        reg2.count++;
                }
        }
        close (ifd);
        printf("Reg2\n");
        displayReg(reg2);
}

int getCount(Register reg) {
        /* returns the number of records in the register reg */
        return (reg.count);
}

int getNthRec(int fd, Record *r, int n) {
        /* Read the n-th record from the file having descriptor fd into record pointed to by r 
                returns 1 if successful
                        0 otherwise
        */
        int rsize; /* record size */
        int posn;  /* position in the file where to write */
        int rdsize; /* how much could be read - it should be same as rsize */

        rsize = sizeof(Record); /* size of each Record */
        posn = n * rsize; /* Position at the starting of the n-th record in the file */


        /* Position at the starting of the n-th record in the file */
        /* off_t lseek(int fd, off_t offset, int whence);*/
        lseek(fd, posn, SEEK_SET);

        /* Read the record from that position */
        /* ssize_t read(int fd, void *buf, size_t count); */
        rdsize = read(fd, (void *)r, rsize);
        if (rdsize < rsize) {
                /* whole record was not read */
                fprintf(stderr, "getNthRec(): read %d of %d bytes.\n", rdsize, rsize);
                fprintf(stderr, "read() Error:");
                return 0;
        }

        return 1;
}
int putNthRec(int fd, Record r, int n) {
        /* Write the record r at the n-th record position in the file with descriptor
                returns 1 if successful
                        0 otherwise
        */
        int rsize; /* record size */
        int posn;  /* position in the file where to write */
        int fposn;  /* stires position returned by lseek() */
        int wsize; /* how much could be written - it should be same as rsize */

        rsize = sizeof(Record); /* size of each Record */
        posn = n * rsize; /* Position at the starting of the n-th record in the file */


        /* Position at the starting of the n-th record in the file */
        /* off_t lseek(int fd, off_t offset, int whence);*/
        fposn = lseek(fd, posn, SEEK_SET);
        if (fposn == -1) {
                perror ("lseek failes:");
        }

        /* Write the record at that position */
        /* ssize_t write(int fd, const void *buf, size_t count); */
        wsize = write(fd, (void *)&r, rsize);
        if (wsize < rsize) {
                /* whole record was not written */
                fprintf(stderr, "putNthRec(): wrote %d of %d bytes.\n", wsize, rsize);
                perror("write() Error:");
                return 0;
        }

        return 1;
}
void displayRec(Record r) {
        /* Displays the record r */
        printf("Record: Field 1: %s, Field 2: %d.\n", r.field1, r.field2);
}
void displayReg(Register reg) {
        /* Displays the records of reg */
        int i;

       for (i=0; i < reg.count; i++) {
                displayRec(reg.rs[i]);
        }

}

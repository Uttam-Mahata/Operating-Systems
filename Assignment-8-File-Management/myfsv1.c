#include <stdio.h>
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

char buf[4096];
char sbuf[8*4096];

int mymkfs(const char *fname);
int mycopyFrom(char *mfname, char *fname);
int mycopyTo(char *fname, char *mfname);
int myrm(char *);
int myreadSBlocks(int fd, char *sbuf);
int mywriteSBlocks(int fd, char *sbuf);
int myreadBlock(int fd, int bno, char *buf);
int mywriteBlock(int fd, int bno, char *buf);

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
flag = mymkfs(argv[1]);
if (flag != 0) {
fprintf(stderr, "%s Failed!\n", argv[0]);
}
} else if (strcmp(basename, "mycopyTo") == 0) {
if (argc != 3) {
fprintf(stderr, "Usage: %s <linux file name> <storage file name>\n", argv[0]);
exit(1);
}
flag = mycopyTo(argv[1], argv[2]);
if (flag != 0) {
fprintf(stderr, "%s Failed!\n", argv[0]);
}
} else if (strcmp(basename, "mycopyFrom") == 0) {
if (argc != 3) {
fprintf(stderr, "Usage: %s <myfile name>@<storage file name> <linux file name>\n", argv[0]);
exit(1);
}
flag = mycopyFrom(argv[1], argv[2]);
if (flag != 0) {
fprintf(stderr, "%s Failed!\n", argv[0]);
}
} else if (strcmp(basename, "myrm") == 0) {
if (argc != 2) {
fprintf(stderr, "Usage: %s <myfile name>@<storage file name>\n", argv[0]);
exit(1);
}
flag = myrm(argv[1]);
if (flag != 0) {
fprintf(stderr, "%s Failed!\n", argv[0]);
}
} else {
fprintf(stderr,"%s: Command not found!\n", argv[0]);
}
}

int mymkfs(const char *fname) {
int fd;
int i;
int flag;
fd = open(fname, O_CREAT | O_WRONLY, S_IRWXU);
if (fd == -1) {
fprintf(stderr,"%s: ", fname);
perror("File cannot be opened for writing");
return (-1);
}
memset(buf, 0, BS);
for (i=0; i < BNO + 8; i++) {
flag = write(fd, buf, BS);
if(flag == -1) {
fprintf(stderr,"%s: ", fname);
perror("File write failed!");
return (-1);
}
}
return (0);
}

int mycopyTo(char *fname, char *mfname) {
int fd;
int fdTo;
int i;
int flag;       
int hole;
struct stat sb;

flag = stat(fname, &sb);
if (flag == -1) {
fprintf(stderr,"File %s ", fname);
perror("stat() failed: ");
return (-1);
}

if (strlen(fname) > FNLEN) {
fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
fprintf(stderr, "Name is longer than %d!\n", FNLEN);
return (-1);
} else if (sb.st_size > BS) {
fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
fprintf(stderr, "File size %d is bigger than %d!\n", (int)(sb.st_size), BS);
return (-1);
}

fd = open(fname, O_RDONLY);
if (fd == -1) {
fprintf(stderr,"%s: ", fname);
perror("Cannot be opened for reading: ");
return (-1);
}

fdTo = open(mfname, O_RDWR);
if (fdTo == -1) {
fprintf(stderr,"%s: ", mfname);
perror("Cannot be opened for writing: ");
return (-1);
}

flag = myreadSBlocks(fdTo, sbuf);

hole = -1;
for (i = 0; i < NOFILES; i++) {
if (sbuf[i*16] == 0) {
hole = i;
}
if (strncmp(fname, &(sbuf[i*16]), FNLEN) == 0) {
break;
}
}

if (i >= NOFILES) {
strcpy(&(sbuf[hole*16]), fname);
*((int *)&(sbuf[hole*16 + 12])) = (int)(sb.st_size);
} else {
fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
fprintf(stderr, "File already exists!\n");
return (-1);
}

flag = read(fd, buf, BS);
if(flag == -1) {
fprintf(stderr,"%s: ", fname);
perror("File read failed!");
return (-1);
}

flag = mywriteBlock(fdTo, 8+hole, buf);
if (flag == -1) {
fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
fprintf(stderr, "mywriteBlock() failed!\n");
return (-1);
}

flag = mywriteSBlocks(fdTo, sbuf);
if (flag == -1) {
fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
fprintf(stderr, "mywriteSBlocks() failed!\n");
close(fdTo);
return (-1);
}
close(fd);
close(fdTo);
return (0);
}

int mycopyFrom(char *mfname, char *fname) {
int fd;
int fdFrom;
int i;
int flag;       
struct stat sb;
char *myfsname;
char *myfilename;
int myfilesize;

myfilename = mfname;
myfsname = strchr(mfname, '@');
if (myfsname != NULL) {
*myfsname = '\0';
myfsname++;
} else {
fprintf(stderr, "%s should be of the form <myfile name>@<myfs file name>\n", mfname);
return(-1);
}

fd = open(fname, O_CREAT | O_WRONLY, S_IRWXU);
if (fd == -1) {
fprintf(stderr,"%s: ", fname);
perror("Cannot be opened for writing");
return (-1);
}

fdFrom = open(myfsname, O_RDWR);
if (fdFrom == -1) {
fprintf(stderr,"%s: ", myfsname);
perror("Cannot be opened for reading: ");
return (-1);
}

flag = myreadSBlocks(fdFrom, sbuf);

for (i = 0; i < NOFILES; i++) {
if (strncmp(myfilename, &(sbuf[i*16]), FNLEN) == 0) {
break;
}
}

if (i >= NOFILES) {
fprintf(stderr, "File %s cannot be found in  myfs on %s!\n", myfilename, myfsname);
return (-1);
}

myfilesize = *((int *)&(sbuf[i*16 + 12]));

flag = myreadBlock(fdFrom, 8+i, buf);
if (flag == -1) {
fprintf(stderr, "File %s cannot be read from myfs on %s!\n", myfilename, myfsname);
fprintf(stderr, "myreadBlock() failed!\n");
return (-1);
}

flag = write(fd, buf, myfilesize);
if(flag == -1) {
fprintf(stderr,"%s: ", fname);
perror("File write failed!");
close(fd);
return (-1);
}

close(fd);
close(fdFrom);
return (0);
}

int myrm(char *mfname) {
int fdFrom;
int i;
int flag;       
char *myfsname;
char *myfilename;

myfilename = mfname;
myfsname = strchr(mfname, '@');
if (myfsname != NULL) {
*myfsname = '\0';
myfsname++;
} else {
fprintf(stderr, "%s should be of the form <myfile name>@<myfs file name>\n", mfname);
return(-1);
}

fdFrom = open(myfsname, O_RDWR);
if (fdFrom == -1) {
fprintf(stderr,"%s: ", myfsname);
perror("Cannot be opened for reading-writing: ");
return (-1);
}

flag = myreadSBlocks(fdFrom, sbuf);

for (i = 0; i < BNO; i++) {
if (strncmp(myfilename, &(sbuf[i*16]), FNLEN) == 0) {
break;
}
}

if (i >= NOFILES) {
fprintf(stderr, "File %s cannot be found in  myfs on %s!\n", myfilename, myfsname);
return (-1);
}

sbuf[i*16] = '\0';
*((int *)&(sbuf[i*16 + 12])) = 0;

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
flag = lseek(fd, 0, SEEK_SET);
if (flag == -1) {
perror("lseek() at myreadSBlocks() fails: ");
return (flag);
}
flag = 0;
for (i = 0; i < 8 && flag != -1; i++) {
flag = myreadBlock(fd, i, &(sbuf[i*BS]));
}
return (flag);
}

int mywriteSBlocks(int fd, char *sbuf) {
int i;
int flag;
flag = lseek(fd, 0, SEEK_SET);
if (flag == -1) {
perror("lseek() at mywriteSBlocks() fails: ");
return (-1);
}
flag = 0;
for (i = 0; i < 8 && flag != -1; i++) {
flag = mywriteBlock(fd, i, &(sbuf[i*BS]));
}
return (flag);
}

int myreadBlock(int fd, int bno, char *buf) {
int flag;
flag = lseek(fd, bno * BS, SEEK_SET);
if (flag == -1) {
perror("lseek() at myreadBlock() fails: ");
return (flag);
}
flag = read(fd, buf, BS);
if (flag == -1) {
perror("read() at myreadBlock() fails: ");
return (-1);
}
return (0);
}

int mywriteBlock(int fd, int bno, char *buf) {
int flag;
flag = lseek(fd, bno * BS, SEEK_SET);
if (flag == -1) {
perror("lseek() at myreadBlock() fails: ");
return (flag);
}
flag = write(fd, buf, BS);
if (flag == -1) {
perror("read() at myreadBlock() fails: ");
return (-1);
}
return (0);
}

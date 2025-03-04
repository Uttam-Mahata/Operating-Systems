/* File: stream1.c ["./a.out abc.txt" prints the number of english letters in the file abc.txt] */
#include <stdio.h>
#include <stdlib.h> /* the manual for exit() function (used in this program) tells that this header file has to be included - "man 3 exit"*/

int main(int argc, char *argv[]) {

	FILE *fp; /* the variable to store the stream for the input file */
	char ch;
	int countE=0;

	if (argc != 2) { 
		/* the program is invoked with wrong number of command line arguments.
		   that is, it could have been called as "./a.out" where argc becomes 1 OR
		   it could have been called as "./countL abc def ghi" where argc becomes 4, etc.
		   So , print an error message and exit from the program
		*/
		printf("Wrong arguments! Use as %s <filename>\n", argv[0]); /* argv[0] is the name of the executable program - Eg., a.out or countL in the above examples. */
		exit(1); /* what does it do? Read manual by "man 3 exit" */
		
	}

	fp = fopen(argv[1], "r"); /* argv[1] stores the name of the file from which to read. "r" stands for "read" mode. */
				  /* for details on fopen() read the online manual - "man 3 fopen" */

	if (fp == NULL) { /* If for some reason fopen cannot "open" the file, it will return NULL) */
		printf("The file %s cannot be opened for reading!!!\n", argv[1]); /* argv[1] is the name of the file the user has given as the command line argument! Eg., For example if the user has executed the program as "./a.out abc.txt", then argv[1] will contain "abc.txt" */
		exit(1); /* what does it do? Read manual by "man 3 exit" */
	}

	while (!feof(fp)) { /* the stream fp has NOT reached "End Of File" - eof. If eof - the loop ends */

		ch = fgetc(fp);
		if ( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) { /* ch is an English letter - either small or capital */
			countE = countE + 1;
		}

	}

	printf("The number of English letters in the file %s is %d.\n", argv[1], countE);
}

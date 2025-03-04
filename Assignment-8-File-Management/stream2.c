/* File: stream2.c ["./a.out abc.txt" would print number of characters, words, lines and sentences in the file abc.txt.] */
#include <stdio.h>
#include <stdlib.h> /* the manual for exit() function (used in this program) tells that this header file has to be included - "man 3 exit"*/
#define true 1
#define false 0
int main(int argc, char *argv[]) {

        FILE *fp; /* the variable to store the stream for the input file */
        char ch; /* the contents of the file will be read in this variable one character by one character */
        int countC=0; /* to store the number of Characters in the file */
        int countW=0; /* to store the number of Words in the file */
        int countL=0; /* to store the number of Lines in the file */
        int countS=0; /* to store the number of Sentences in the file */
        
        /* Identifying a new word is a bit tricky comprared to identifying lines or sentences.
           A non-white character (non-space and non-tab and non-new-line) after a sequence of white characters signifies starting of a new word.
           That is, if the previous character is white, then a non-white character means starting of a word.
           We shall keep track of this using a variable isPreviousWhte */
        int isPreviousWhite = true; /* at the begining of the file, a non-white character (even if it is not 
                                       preceded by white characters) means starting of a new word */
        if (argc != 2) {
                /* the program is invoked with wrong number of command line arguments.
                   that is, it could have been called as "./a.out" where argc becomes 1 OR
                   it could have been called as "./countL abc def ghi" where argc becomes 4, etc.
                   So , print an error message and exit from the program
                */
                printf("Wrong arguments! Use as %s <filename>\n", argv[0]); /* argv[0] is the name of the executable program - Eg., a.out or countL in the above examples. */
                exit(1); /* What does it do? Read manual by "man 3 exit" */
    
        }

        fp = fopen(argv[1], "r"); /* argv[1] stores the name of the file from which to read. "r" stands for "read" mode. */
                                  /* for details on fopen() read the online manual - "man 3 fopen" */

        if (fp == NULL) { /* If for some reason fopen cannot "open" the file, it will return NULL) */
                printf("The file %s cannot be opened for reading!!!\n", argv[1]); /* argv[1] is the name of the file the user has given as the command line argument! Eg., For example if the user has executed the program as "./a.out abc.txt", then argv[1] will contain "abc.txt" */
                exit(1); /* what does it do? Read manual by "man 3 exit" */
        }

        while (!feof(fp)) { /* the stream fp has NOT reached "End Of File" - eof. If eof - the loop ends */

                ch = fgetc(fp); /* read a nex character from the file */
                
                countC++; /* a new character is read */
                
                if (ch == '\n') { /* there is a new line to start now */
                				countL++; /* a new line is starting */
                }
                
                if (ch == '.') { /* this is the end of a new sentence */
                				countS++; /* a new line is starting */
                }
                
                if (ch == ' ' || ch == '\t' || ch == '\n') { /* the new character is a white character */
                				isPreviousWhite = true;
                } else { /* the new character is non-white */
                				if ( isPreviousWhite == true ) { /* the previous character was white and 
                        																		the new character is non-white - it is a new word then */
                        					countW++;
                                  isPreviousWhite = false;
                        }
                        	
                }
        }           
        printf("In the file %s the number of characters = %d, number of word = %d, number of lines = %d, and the number of sentences = %d.\n", argv[1], countC, countW, countL, countS);
}

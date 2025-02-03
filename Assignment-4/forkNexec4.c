// This is an extension of the previous assignment titled "Child Processes execute different executable files as "told" by the user: Problem 1." Hence, you may please edit the final file (say, forkNexec3.c) for the present assignment.

// Please note that often programs take command line arguments (Eg., the programs you write having a main() function defined as "int main(int argc, char *argv[]) {.....}". For example, the unix command "ls" (/bin/ls being the executable file) can be executed as "ls -l" ("-l" is the command line argument) producing an output like:
// total 488
// -rw-rw-r-- 1 manas manas 486286 Nov 10  2022 8086.zip
// drwxrwxr-x 2 manas manas   4096 Nov 21  2022 ASM
// drwx------ 2 manas manas   4096 Nov 10  2022 bin

// or can be executed as "ls -l 8086*" ("-l" and "8086*" being the command line arguments) producing output like:
// -rw-rw-r-- 1 manas manas 486286 Nov 10  2022 8086.zip

// In the present assignment, the program, in every iteration of the infinite-loop, reads a whole line, where the 1st word of the line is to be treated as the name of an executable file, and the remaining words are to be treated as the command line arguments to the executable file. And the child process, now, should execute the file by passing those command line arguments.


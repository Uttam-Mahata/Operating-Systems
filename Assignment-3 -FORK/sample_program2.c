#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
pid_t p;

printf("Hello World\n"); // Executed by parent process

p = fork();

if(p == -1) {
printf("Fork Failed\n");

}
/*
In the parent process the return value from fork() will be the process id (pid) of the child process.
For child process return value from fork() will be 0.

*/

if(p == 0) {
printf("Child Process\n");
} else {
printf("Parent Process\n");
}

printf("Executed by both Parent and Child Process\n");

return 0;

}

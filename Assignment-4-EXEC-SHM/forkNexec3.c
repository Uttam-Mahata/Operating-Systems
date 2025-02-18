#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>


int main(int argc, char *argv[]) {

// int status;
// char *myargv[]={"/bin/ls", NULL};
// int execve(const char *filename, char *const argv[], char *const envp[]);
// status = execve("/bin/ls", myargv, NULL);
// if (status == -1) {
// perror ("Exec Fails: ");
// }

int status;




    while(1) {
        // Reading a string 
        char str[50];

        printf("Enter the executable file name:");

        scanf("%s", str);
        pid_t pid;

        pid_t p = fork();

        if(p == -1) {
            printf("Error Failed");
            exit(1);
        }

        // Child Process

        if(p == 0) {


            int status;
            char *myargv[]={str, NULL};
            // int execve(const char *filename, char *const argv[], char *const envp[]);
            status = execve(str, myargv, NULL);
            if (status == -1) {
                perror ("Exec Fails: ");
                exit(9);
            }
       






        } else {
        pid = wait(&status);

        if(WIFSIGNALED(status)) {
            printf("Child process was terminated by signal %d\n", WTERMSIG(status));
        } else {

        printf("Parent: Child exited with status %d\n", WEXITSTATUS(status));

        }



    }
    return 0;
}
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

int main() {

    // Taking 10 integerts as input

    int n = 10;
    int arr[n];

    printf("Enter 10 integers:\n");

    for (int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }   

    // Creating shared memory to child process

    pid_t pid;
    int status;
    int shmid;

    shmid = shmget(IPC_PRIVATE, sizeof(int) * n, IPC_CREAT | 0777);

    // Shareing the values to child process

    for (int i = 0; i < n; i++) {
        pid = fork();
    

        if (pid == 0) {
            int *shared = (int *) shmat(shmid, NULL, 0);
            shared[i] = arr[i];
            printf("Child %d: %d\n", i, arr[i]);

            shmdt(shared);
            // printf("Child %d: %d\n", i, arr[i]);
            exit(0);
        }
    }

  
}
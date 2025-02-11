#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>



struct task {
    char data[100];
    pid_t worker_pid;
    int status;
};

int main() {
    key_t shmkey;
    int shmid;
    struct task *solve;

    // Generation of a shared memory key
    shmkey = ftok("/home/uttammahata/projects", 1);
    if (shmkey == -1) {
        perror("Worker: ftok failed");
        exit(1);
    }

    // Creation of a shared memory segment
    shmid = shmget(shmkey, sizeof(struct task), 0666);
    if (shmid == -1) {
        perror("Worker: shmget failed");
        exit(1);
    }

    printf("Worker: Shared memory ID: %d\n", shmid);

    // Attaching shared memory segment
    solve = (struct task *)shmat(shmid, NULL, 0);
    if (solve == (struct task *)-1) {
        perror("Worker: shmat failed");
        exit(1);
    }

    printf("Worker: Attached to shared memory.\n");

    while (solve->status != -1) {
        // Wait for server to put data
        while (solve->status != 1 && solve->status != -1) {
            sleep(1);
        }

        if (solve->status == -1) break; // Exit if server is terminating

        // Computing the length of the string
        solve->status = 2;
        solve->worker_pid = getpid();
        printf("Worker PID %d: Computing length of string: %s\n", solve->worker_pid, solve->data);

        int length = strlen(solve->data);

        sprintf(solve->data, "%d", length);

        solve->status = 3;
        printf("Worker PID %d: length computation done.\n", solve->worker_pid);

        
    }

    printf("Worker PID %d: Exiting.\n", getpid());
    shmdt(solve); // Detach the shared memory segment
    return 0;
}
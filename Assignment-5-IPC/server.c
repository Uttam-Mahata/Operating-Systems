#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>


struct task {
    char data[100];
    pid_t worker_pid;
    int status;
};

int shmid;

struct task *solve;

void cleanup(int sig) {
    printf("Received signal is %d .Cleaning up and exiting on Ctrl+C...\n", sig);
    solve->status = -1;

    exit(0);

}

int main(int argc, char* argv[]) {
    key_t shmkey;


    signal(SIGINT, cleanup);

    // Random number generator
    srand(time(NULL));

   // key_t ftok(const char *pathname, int proj_id);


    shmkey = ftok("/home/uttammahata/projects", 1);

    if(shmkey == -1) {
        perror("ftok failed");
        exit(1);
    }

    //    int shmget(key_t key, size_t size, int shmflg);

    // Creation shared memory segment
    shmid = shmget(shmkey, sizeof(struct task), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Server: shmget failed");
        exit(1);
    }
    // Attaching shared memory segment
    solve = (struct task *)shmat(shmid, NULL, 0);
    if (solve == (struct task *)-1) {
        perror("shmat failed");
        exit(1);
    }

    struct shmid_ds shm_stat;

    // Getting status of shared memory

    if (shmctl(shmid, IPC_STAT, &shm_stat) == -1) {
        perror("shmctl failed to get status");
        exit(1);
    }

    if(shm_stat.shm_nattch == 0) {
        solve->status = 0;
        printf("Shared memory initialized.\n");
    }

    else {
        printf("Shared memory already exists.");
    }

     while (1) {
        // Generation of a random string
        int len = (rand() % 20) + 1;
        for (int i = 0; i < len; i++) {
            solve->data[i] = 'a' + (rand() % 26);
        }
        solve->data[len] = '\0';

        printf("Generated string: %s\n", solve->data);
        solve->status = 1;

        // Wait for worker to compute the length
        while (solve->status != 3 && solve->status != -1) {
            sleep(1);
        }

        if (solve->status == -1) break; // Exit if server is terminating

        // Use the computed length
        printf("Worker PID %d computed length: %s\n", solve->worker_pid, solve->data);
        solve->status = 4;

        sleep(2); // Wait before generating the next string

        solve->status = 0; // Reset status for the next task
    }

    printf("Server: Exiting main loop.\n");
    shmdt(solve);  // Detach the shared memory segment




    return 0;

}
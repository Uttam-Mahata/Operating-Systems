#include <stdio.h>
#include <sys/types.h> /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/ipc.h> /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/sem.h> /* for semget(2) semop(2) semctl(2) */
#include <unistd.h> /* for fork(2) */

#include <stdlib.h> /* for exit(3) */
#include <string.h>


#define NO_SEM	1

#define P(s) semop(s, &Pop, 1);
#define V(s) semop(s, &Vop, 1);



struct sembuf Pop;
struct sembuf Vop;

int main(int argc, char *argv[]) {
    key_t mykey;
	pid_t pid;

    int  no_sem = atoi(argv[4]);
    int semnum = no_sem;

    

	int semid;

	int status;
    union semun {
		int              val;    /* Value for SETVAL */
		struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
		unsigned short  *array;  /* Array for GETALL, SETALL */
		struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
	} setvalArg;

    if((strcmp(argv[3], "set") == 0)) {
        setvalArg.val = atoi(argv[5]);
    }

	// setvalArg.val = atoi(argv[5]);

    switch (argc) {
        case 6:
        if (strcmp(argv[3], "set") == 0) {
                // Set a semaphore value
                printf("Setting a semaphore value.\n");

        }

        case 5:
            if (strcmp(argv[3], "create") == 0) {
                // Create a semaphore set
                printf("Creating a semaphore set.\n");

                
            } else if (strcmp(argv[3], "get") == 0) {
                // Get a semaphore value
                printf("Getting a semaphore value.\n");
            } else if (strcmp(argv[3], "inc") == 0) {
                // Increment a semaphore value
                printf("Incrementing a semaphore value.\n");
            } else if (strcmp(argv[3], "dcr") == 0) {
                // Decrement a semaphore value
                printf("Decrementing a semaphore value.\n");
            } else {
                printf("Invalid operation.\n");
            }
            break;
        case 4:
            if (strcmp(argv[3], "rm") == 0) {
                // Remove a semaphore set
                printf("Removing a semaphore set.\n");
            } else if (strcmp(argv[3], "listp") == 0) {
                // List processes waiting on a semaphore
                printf("Listing processes waiting on a semaphore.\n");
            } else {
                printf("Invalid operation.\n");
            }
            break;
        default:
            printf("Invalid number of arguments.\n");
            break;
    }




	/* struct sembuf has the following fields */
	//unsigned short sem_num;  /* semaphore number */
        //short          sem_op;   /* semaphore operation */
        //short          sem_flg;  /* operation flags */

	Pop.sem_num = 0;
	Pop.sem_op = -1;
	Pop.sem_flg = SEM_UNDO;

	Vop.sem_num = 0;
	Vop.sem_op = 1;
	Vop.sem_flg = SEM_UNDO;
    
    // Creation of Semaphore set
    if (strcmp(argv[3], "create") == 0) {
        mykey = ftok(argv[1], atoi(argv[2]));
        if (mykey == -1) {
            perror("ftok() failed");
            exit(1);
        }

        semid = semget(mykey, NO_SEM, IPC_CREAT | 0777);
        if(semid == -1) {
            perror("semget() failed");
            exit(1);
        }
    }

    // Set a semaphore value

    if (strcmp(argv[3], "set") == 0) {
        mykey = ftok(argv[1], atoi(argv[2]));
        if (mykey == -1) {
            perror("ftok() failed");
            exit(1);
        }

        semid = semget(mykey, semnum, IPC_CREAT | 0777);
        if(semid == -1) {
            perror("semget() failed");
            exit(1);
        }

        status = semctl(semid, 0, SETVAL, setvalArg);
        if(status == -1) {
            perror("semctl() failed");
            exit(1);
        }
    }
// Get the semaphore value
    if (strcmp(argv[3], "get") == 0) {
        mykey = ftok(argv[1], atoi(argv[2]));
        if (mykey == -1) {
            perror("ftok() failed");
            exit(1);
        }

        semid = semget(mykey, semnum, IPC_CREAT | 0777);
        if(semid == -1) {
            perror("semget() failed");
            exit(1);
        }

        status = semctl(semid, 0, GETVAL);
        if(status == -1) {
            perror("semctl() failed");
            exit(1);
        }
        printf("Semaphore value: %d\n", status);
    }

// Increment the semaphore value
    if (strcmp(argv[3], "inc") == 0) {
        mykey = ftok(argv[1], atoi(argv[2]));
        if (mykey == -1) {
            perror("ftok() failed");
            exit(1);
        }

        semid = semget(mykey, semnum, IPC_CREAT | 0777);
        if(semid == -1) {
            perror("semget() failed");
            exit(1);
        }

        status = semctl(semid, 0, GETVAL);
        if(status == -1) {
            perror("semctl() failed");
            exit(1);
        }
        printf("Semaphore value: %d\n", status);

        status = semctl(semid, 0, SETVAL, status + atoi(argv[5]));
        if(status == -1) {
            perror("semctl() failed");
            exit(1);
        }
    }

    // Decrement the semaphore value
    if (strcmp(argv[3], "dcr") == 0) {
        mykey = ftok(argv[1], atoi(argv[2]));
        if (mykey == -1) {
            perror("ftok() failed");
            exit(1);
        }

        semid = semget(mykey, semnum, IPC_CREAT | 0777);
        if(semid == -1) {
            perror("semget() failed");
            exit(1);
        }

        status = semctl(semid, 0, GETVAL);
        if(status == -1) {
            perror("semctl() failed");
            exit(1);
        }
        printf("Semaphore value: %d\n", status);

        status = semctl(semid, 0, SETVAL, status - atoi(argv[5]));
        if(status == -1) {
            perror("semctl() failed");
            exit(1);
        }
    }   
    // Remove the semaphore set

    if (strcmp(argv[3], "rm") == 0) {
        mykey = ftok(argv[1], atoi(argv[2]));
        if (mykey == -1) {
            perror("ftok() failed");
            exit(1);
        }

        semid = semget(mykey, semnum, IPC_CREAT | 0777);
        if(semid == -1) {
            perror("semget() failed");
            exit(1);
        }

        status = semctl(semid, 0, IPC_RMID);
        if(status == -1) {
            perror("semctl() failed");
            exit(1);
        }
    }



    



    return 0;
}
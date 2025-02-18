#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* 
 * Union semun is used for semctl operations
 */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};


void print_usage() {
    printf("Usage:\n");
    printf("Create: ./semDemo <file_name> <project_id> create <semnum>\n");
    printf("Set: ./semDemo <file_name> <project_id> set <semnum> <sem_val>\n");
    printf("Get: ./semDemo <file_name> <project_id> get <semnum>\n");
    printf("Increment: ./semDemo <file_name> <project_id> inc <semnum> <val>\n");
    printf("Decrement: ./semDemo <file_name> <project_id> dcr <semnum> <val>\n");
    printf("Remove: ./semDemo <file_name> <project_id> rm\n");
    printf("List processes: ./semDemo <file_name> <project_id> listp <semnum>\n");
}

/*
 * Creates or gets a semaphore ID based on the provided parameters
 */
int get_semaphore_id(const char *filename, int project_id, int semnum, int create_flag) {
    key_t key = ftok(filename, project_id);
    if (key == -1) {
        perror("ftok() failed");
        exit(1);
    }

    int flags = 0666;
    if (create_flag) {
        flags |= IPC_CREAT;
    }
    /* int semget(key_t key, int nsems, int semflg);
    
    DESCRIPTION
           The  semget() system call returns the System V semaphore set identifier
           associated with the argument key. */
           
    int semid = semget(key, semnum, flags);
    if (semid == -1) {
        perror("semget() failed");
        exit(1);
    }

    return semid;
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        print_usage();
        return 1;
    }

    const char *filename = argv[1];
    int project_id = atoi(argv[2]);
    const char *operation = argv[3];
    int semid, status;
    union semun arg;

    if (strcmp(operation, "create") == 0) {
        /* 
         * Create Operation
         * Creates a new semaphore set with specified number of semaphores
         */
        if (argc != 5) {
            printf("Error: Create operation requires semnum\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        semid = get_semaphore_id(filename, project_id, semnum, 1);
        printf("Created semaphore set with ID: %d\n", semid);
    }
    else if (strcmp(operation, "set") == 0) {
        /*
         * Set Operation
         * Sets the value of a specific semaphore in the set
         * Requires semaphore number and new value
         */
        if (argc != 6) {
            printf("Error: Set operation requires semnum and sem_val\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        arg.val = atoi(argv[5]);
        semid = get_semaphore_id(filename, project_id, semnum, 0);
        if (semctl(semid, semnum - 1, SETVAL, arg) == -1) {
            perror("semctl SETVAL failed");
            return 1;
        }
        printf("Set semaphore %d to value %d\n", semnum, arg.val);
    }
    else if (strcmp(operation, "get") == 0) {
        /*
         * Get Operation
         * Retrieves the current value of a specific semaphore
         */
        if (argc != 5) {
            printf("Error: Get operation requires semnum\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        semid = get_semaphore_id(filename, project_id, semnum, 0);
        int value = semctl(semid, semnum - 1, GETVAL);
        if (value == -1) {
            perror("semctl GETVAL failed");
            return 1;
        }
        printf("Semaphore %d value: %d\n", semnum, value);
    }
    else if (strcmp(operation, "inc") == 0) {
        /*
         * Increment Operation
         * Increases semaphore value by specified amount
         * Uses semop() with positive sem_op value
         */
        if (argc != 6) {
            printf("Error: Increment operation requires semnum and val\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        int val = atoi(argv[5]);
        semid = get_semaphore_id(filename, project_id, semnum, 0);
        
        struct sembuf sop = {
            .sem_num = semnum - 1,
            .sem_op = val,
            .sem_flg = 0
        };
        
        if (semop(semid, &sop, 1) == -1) {
            perror("semop increment failed");
            return 1;
        }
        printf("Incremented semaphore %d by %d\n", semnum, val);
    }
    else if (strcmp(operation, "dcr") == 0) {
        /*
         * Decrement Operation
         * Decreases semaphore value by specified amount
         * Uses semop() with negative sem_op value
         */
        if (argc != 6) {
            printf("Error: Decrement operation requires semnum and val\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        int val = atoi(argv[5]);
        semid = get_semaphore_id(filename, project_id, semnum, 0);
        
        struct sembuf sop = {
            .sem_num = semnum - 1,
            .sem_op = -val,
            .sem_flg = 0
        };
        
        if (semop(semid, &sop, 1) == -1) {
            perror("semop decrement failed");
            return 1;
        }
        printf("Decremented semaphore %d by %d\n", semnum, val);
    }
    else if (strcmp(operation, "rm") == 0) {
        /*
         * Remove Operation
         * Removes the entire semaphore set using IPC_RMID command
         */
        if (argc != 4) {
            printf("Error: Remove operation takes no additional arguments\n");
            return 1;
        }
        semid = get_semaphore_id(filename, project_id, 1, 0);
        if (semctl(semid, 0, IPC_RMID) == -1) {
            perror("semctl IPC_RMID failed");
            return 1;
        }
        printf("Removed semaphore set\n");
    }
    else if (strcmp(operation, "listp") == 0) {
        /*
         * List Processes Operation
         * Shows information about processes waiting on the semaphore
         * Uses IPC_STAT to get semaphore status information
         */
        if (argc != 5) {
            printf("Error: List processes operation requires semnum\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        semid = get_semaphore_id(filename, project_id, semnum, 0);
        
        // Get semaphore information
        struct semid_ds sem_info;
        arg.buf = &sem_info;
        
        if (semctl(semid, semnum - 1, IPC_STAT, arg) == -1) {
            perror("semctl IPC_STAT failed");
            return 1;
        }
        
        printf("Number of processes waiting for semaphore %d to become greater: %lu\n", 
               semnum, sem_info.sem_otime);
        printf("Number of processes waiting for semaphore %d to become zero: %lu\n", 
               semnum, sem_info.sem_ctime);
    }
    else {
        printf("Unknown operation: %s\n", operation);
        print_usage();
        return 1;
    }

    return 0;
}
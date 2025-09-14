/**
 * @file semDemoNew.c
 * @brief A command-line tool for managing System V semaphores.
 * This program provides a set of commands to create, set, get, increment, decrement, and remove semaphores, as well as list waiting processes.
 * This is an improved version of semDemo.c with better structure and error handling.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/**
 * @union semun
 * @brief A union for semaphore control operations. This is required for semctl().
 */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

/**
 * @brief Prints the usage information for the program.
 */
void print_usage() {
    printf("Usage:\n");
    printf("  Create: ./semDemoNew <file_name> <project_id> create <semnum>\n");
    printf("  Set:    ./semDemoNew <file_name> <project_id> set <semnum> <sem_val>\n");
    printf("  Get:    ./semDemoNew <file_name> <project_id> get <semnum>\n");
    printf("  Inc:    ./semDemoNew <file_name> <project_id> inc <semnum> <val>\n");
    printf("  Dec:    ./semDemoNew <file_name> <project_id> dcr <semnum> <val>\n");
    printf("  Remove: ./semDemoNew <file_name> <project_id> rm\n");
    printf("  List:   ./semDemoNew <file_name> <project_id> listp <semnum>\n");
}

/**
 * @brief Creates or gets a semaphore ID based on the provided parameters.
 * @param filename The pathname for ftok().
 * @param project_id The project ID for ftok().
 * @param semnum The number of semaphores in the set.
 * @param create_flag A flag to indicate whether to create the semaphore set if it doesn't exist.
 * @return The semaphore ID.
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
    
    int semid = semget(key, semnum, flags);
    if (semid == -1) {
        perror("semget() failed");
        exit(1);
    }

    return semid;
}

/**
 * @brief The main function. It parses the command-line arguments and performs the requested semaphore operation.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        print_usage();
        return 1;
    }

    const char *filename = argv[1];
    int project_id = atoi(argv[2]);
    const char *operation = argv[3];
    int semid;
    union semun arg;

    if (strcmp(operation, "create") == 0) {
        if (argc != 5) {
            printf("Error: Create operation requires <semnum>\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        semid = get_semaphore_id(filename, project_id, semnum, 1);
        printf("Created semaphore set with ID: %d\n", semid);
    } else if (strcmp(operation, "set") == 0) {
        if (argc != 6) {
            printf("Error: Set operation requires <semnum> and <sem_val>\n");
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
    } else if (strcmp(operation, "get") == 0) {
        if (argc != 5) {
            printf("Error: Get operation requires <semnum>\n");
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
    } else if (strcmp(operation, "inc") == 0) {
        if (argc != 6) {
            printf("Error: Increment operation requires <semnum> and <val>\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        int val = atoi(argv[5]);
        semid = get_semaphore_id(filename, project_id, semnum, 0);
        
        struct sembuf sop = { .sem_num = semnum - 1, .sem_op = val, .sem_flg = 0 };
        
        if (semop(semid, &sop, 1) == -1) {
            perror("semop increment failed");
            return 1;
        }
        printf("Incremented semaphore %d by %d\n", semnum, val);
    } else if (strcmp(operation, "dcr") == 0) {
        if (argc != 6) {
            printf("Error: Decrement operation requires <semnum> and <val>\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        int val = atoi(argv[5]);
        semid = get_semaphore_id(filename, project_id, semnum, 0);
        
        struct sembuf sop = { .sem_num = semnum - 1, .sem_op = -val, .sem_flg = 0 };
        
        if (semop(semid, &sop, 1) == -1) {
            perror("semop decrement failed");
            return 1;
        }
        printf("Decremented semaphore %d by %d\n", semnum, val);
    } else if (strcmp(operation, "rm") == 0) {
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
    } else if (strcmp(operation, "listp") == 0) {
        if (argc != 5) {
            printf("Error: List processes operation requires <semnum>\n");
            return 1;
        }
        int semnum = atoi(argv[4]);
        semid = get_semaphore_id(filename, project_id, semnum, 0);
        
        struct semid_ds sem_info;
        arg.buf = &sem_info;
        
        if (semctl(semid, semnum - 1, IPC_STAT, arg) == -1) {
            perror("semctl IPC_STAT failed");
            return 1;
        }
        
        printf("Number of processes waiting for semaphore %d to become greater: %ld\n", semnum, sem_info.sem_otime);
        printf("Number of processes waiting for semaphore %d to become zero: %ld\n", semnum, sem_info.sem_ctime);
    } else {
        printf("Unknown operation: %s\n", operation);
        print_usage();
        return 1;
    }

    return 0;
}
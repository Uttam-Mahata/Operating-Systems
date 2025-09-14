/**
 * @file semDemo.c
 * @brief A command-line tool for managing System V semaphores.
 * @note This program has some bugs and is not well-structured. For a better version, see semDemoNew.c.
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
 * @brief A union for semaphore control operations.
 */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

/**
 * @brief The main function. It parses the command-line arguments and performs the requested semaphore operation.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <file> <proj_id> <operation> [args...]\n", argv[0]);
        exit(1);
    }

    key_t mykey;
    int semid;
    int status;
    union semun setvalArg;

    // This is not a robust way to parse arguments.
    int no_sem = (argc > 4) ? atoi(argv[4]) : 1;
    int semnum = no_sem;

    if (argc > 5 && (strcmp(argv[3], "set") == 0)) {
        setvalArg.val = atoi(argv[5]);
    }

    mykey = ftok(argv[1], atoi(argv[2]));
    if (mykey == -1) {
        perror("ftok() failed");
        exit(1);
    }

    // The logic here is flawed. It tries to get a semaphore set with a variable number of semaphores,
    // which will not work if the set already exists with a different number.
    semid = semget(mykey, no_sem, IPC_CREAT | 0777);
    if (semid == -1) {
        perror("semget() failed");
        exit(1);
    }

    if (strcmp(argv[3], "create") == 0) {
        printf("Semaphore set created with id %d\n", semid);
    } else if (strcmp(argv[3], "set") == 0) {
        status = semctl(semid, semnum - 1, SETVAL, setvalArg);
        if (status == -1) {
            perror("semctl() failed");
            exit(1);
        }
    } else if (strcmp(argv[3], "get") == 0) {
        status = semctl(semid, semnum - 1, GETVAL);
        if (status == -1) {
            perror("semctl() failed");
            exit(1);
        }
        printf("Semaphore value: %d\n", status);
    } else if (strcmp(argv[3], "inc") == 0) {
        struct sembuf sop;
        sop.sem_num = semnum - 1;
        sop.sem_op = (argc > 5) ? atoi(argv[5]) : 1;
        sop.sem_flg = 0;
        if (semop(semid, &sop, 1) == -1) {
            perror("semop() failed");
            exit(1);
        }
    } else if (strcmp(argv[3], "dcr") == 0) {
        struct sembuf sop;
        sop.sem_num = semnum - 1;
        sop.sem_op = (argc > 5) ? -atoi(argv[5]) : -1;
        sop.sem_flg = 0;
        if (semop(semid, &sop, 1) == -1) {
            perror("semop() failed");
            exit(1);
        }
    } else if (strcmp(argv[3], "rm") == 0) {
        status = semctl(semid, 0, IPC_RMID);
        if (status == -1) {
            perror("semctl() failed");
            exit(1);
        }
    } else {
        fprintf(stderr, "Invalid operation: %s\n", argv[3]);
        exit(1);
    }

    return 0;
}
Semaphores are a powerful synchronization mechanism used to coordinate access to shared resources among multiple processes or threads. They can be used in various scenarios to ensure mutual exclusion, manage resource allocation, and synchronize processes. Below, I explain how semaphores can be used in different scenarios and provide C code examples for each.

---

### **Scenarios for Using Semaphores**

1. **Mutual Exclusion (Binary Semaphore)**:
   - Ensure that only one process accesses a critical section of code at a time.
   - Example: Protecting shared variables or data structures.

2. **Producer-Consumer Problem (Counting Semaphore)**:
   - Synchronize processes where one produces data and another consumes it.
   - Example: A bounded buffer shared between producer and consumer processes.

3. **Reader-Writer Problem**:
   - Allow multiple readers to access a resource simultaneously but ensure exclusive access for writers.
   - Example: A shared database or file.

4. **Barrier Synchronization**:
   - Ensure that a group of processes reach a certain point before proceeding.
   - Example: Parallel computation where processes must synchronize at specific stages.

---

### **Mechanism and C Code Examples**

#### **1. Mutual Exclusion (Binary Semaphore)**

**Scenario**:
- Two processes need to update a shared variable without causing race conditions.

**Mechanism**:
- Use a binary semaphore (`mutex`) to lock the critical section.

**C Code Example**:
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>

// Semaphore operations
void sem_wait(int semid) {
    struct sembuf sb = {0, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }
}

void sem_signal(int semid) {
    struct sembuf sb = {0, 1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("sem_signal failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int shmid, semid;
    int *shared_var;

    // Create shared memory
    shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    shared_var = (int *)shmat(shmid, NULL, 0);
    if (shared_var == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    // Initialize shared variable
    *shared_var = 0;

    // Create semaphore
    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphore
    semctl(semid, 0, SETVAL, 1); // mutex = 1

    // Fork a child process
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        for (int i = 0; i < 5; i++) {
            sem_wait(semid); // Lock
            (*shared_var)++;
            printf("Child: shared_var = %d\n", *shared_var);
            sem_signal(semid); // Unlock
            sleep(1);
        }
    } else { // Parent process
        for (int i = 0; i < 5; i++) {
            sem_wait(semid); // Lock
            (*shared_var)++;
            printf("Parent: shared_var = %d\n", *shared_var);
            sem_signal(semid); // Unlock
            sleep(1);
        }
        wait(NULL); // Wait for child to finish
    }

    // Cleanup
    shmdt(shared_var);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}
```

---

#### **2. Producer-Consumer Problem (Counting Semaphore)**

**Scenario**:
- A producer process adds items to a shared buffer, and a consumer process removes them.

**Mechanism**:
- Use two counting semaphores:
  - `empty`: Tracks the number of empty slots in the buffer.
  - `full`: Tracks the number of filled slots in the buffer.
- Use a binary semaphore (`mutex`) to protect the buffer.

**C Code Example**:
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>

#define BUFFER_SIZE 5

// Semaphore operations
void sem_wait(int semid, int sem_num) {
    struct sembuf sb = {sem_num, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }
}

void sem_signal(int semid, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("sem_signal failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int shmid, semid;
    int *buffer;

    // Create shared memory
    shmid = shmget(IPC_PRIVATE, BUFFER_SIZE * sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    buffer = (int *)shmat(shmid, NULL, 0);
    if (buffer == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    // Create semaphores
    semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores
    semctl(semid, 0, SETVAL, 1); // mutex = 1
    semctl(semid, 1, SETVAL, BUFFER_SIZE); // empty = BUFFER_SIZE
    semctl(semid, 2, SETVAL, 0); // full = 0

    // Fork a child process (consumer)
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Consumer process
        for (int i = 0; i < 10; i++) {
            sem_wait(semid, 2); // Wait for full
            sem_wait(semid, 0); // Lock mutex

            int item = buffer[i % BUFFER_SIZE];
            printf("Consumed: %d\n", item);

            sem_signal(semid, 0); // Unlock mutex
            sem_signal(semid, 1); // Signal empty
            sleep(1);
        }
    } else { // Producer process
        for (int i = 0; i < 10; i++) {
            sem_wait(semid, 1); // Wait for empty
            sem_wait(semid, 0); // Lock mutex

            buffer[i % BUFFER_SIZE] = i;
            printf("Produced: %d\n", i);

            sem_signal(semid, 0); // Unlock mutex
            sem_signal(semid, 2); // Signal full
            sleep(1);
        }
        wait(NULL); // Wait for child to finish
    }

    // Cleanup
    shmdt(buffer);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}
```

---

#### **3. Reader-Writer Problem**

**Scenario**:
- Multiple readers can read a shared resource simultaneously, but only one writer can write at a time.

**Mechanism**:
- Use a binary semaphore (`rw_mutex`) to protect the resource.
- Use a counting semaphore (`read_count_mutex`) to protect the reader count.

**C Code Example**:
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>

// Semaphore operations
void sem_wait(int semid, int sem_num) {
    struct sembuf sb = {sem_num, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }
}

void sem_signal(int semid, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("sem_signal failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int shmid, semid;
    int *shared_data;
    int *read_count;

    // Create shared memory
    shmid = shmget(IPC_PRIVATE, sizeof(int) + sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    shared_data = (int *)shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    read_count = shared_data + 1;

    // Initialize shared data
    *shared_data = 0;
    *read_count = 0;

    // Create semaphores
    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores
    semctl(semid, 0, SETVAL, 1); // rw_mutex = 1
    semctl(semid, 1, SETVAL, 1); // read_count_mutex = 1

    // Fork a reader process
    pid_t reader_pid = fork();
    if (reader_pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (reader_pid == 0) { // Reader process
        for (int i = 0; i < 5; i++) {
            sem_wait(semid, 1); // Lock read_count_mutex
            (*read_count)++;
            if (*read_count == 1) {
                sem_wait(semid, 0); // Lock rw_mutex
            }
            sem_signal(semid, 1); // Unlock read_count_mutex

            printf("Reader: data = %d\n", *shared_data);

            sem_wait(semid, 1); // Lock read_count_mutex
            (*read_count)--;
            if (*read_count == 0) {
                sem_signal(semid, 0); // Unlock rw_mutex
            }
            sem_signal(semid, 1); // Unlock read_count_mutex
            sleep(1);
        }
    } else { // Writer process
        for (int i = 0; i < 5; i++) {
            sem_wait(semid, 0); // Lock rw_mutex

            (*shared_data)++;
            printf("Writer: data = %d\n", *shared_data);

            sem_signal(semid, 0); // Unlock rw_mutex
            sleep(1);
        }
        wait(NULL); // Wait for reader to finish
    }

    // Cleanup
    shmdt(shared_data);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}
```

---

### **Summary**

- **Binary Semaphore**: Used for mutual exclusion (e.g., protecting critical sections).
- **Counting Semaphore**: Used for resource management (e.g., producer-consumer problem).
- **Reader-Writer Semaphore**: Used to allow concurrent reads but exclusive writes.

These examples demonstrate how semaphores can be used to synchronize processes in different scenarios. Proper cleanup of shared memory and semaphores is essential to avoid resource leaks.

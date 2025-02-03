#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

// Structure to hold shared data
struct shared_data {
    int number;           // The shared number
    int parent_written;   // Flag to indicate parent has written
    int child_written;    // Flag to indicate child has written
};

// Function to calculate factorial
long long factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    key_t key = ftok(".", 'a');  // Generate unique key
    int shmid = shmget(key, sizeof(struct shared_data), IPC_CREAT | 0666);
    
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    
    // Attach shared memory
    struct shared_data *shared = (struct shared_data *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }
    
    // Initialize shared data
    shared->parent_written = 0;
    shared->child_written = 1;  // Allow parent to write first
    
    // Fork process
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    
    if (pid > 0) {  // Parent process
        srand(time(NULL));  // Initialize random seed
        
        for (int i = 0; i < 10; i++) {  // Run 10 iterations
            // Wait until child has processed previous number
            while (!shared->child_written) {
                usleep(100000);  // Sleep for 100ms
            }
            
            // Generate and write random number
            int random_num = rand() % 10 + 1;  // Random number between 1 and 10
            shared->number = random_num;
            shared->parent_written = 1;
            shared->child_written = 0;
            
            printf("Parent: Generated number %d\n", random_num);
            
            // Wait for child to calculate factorial
            while (!shared->child_written) {
                usleep(100000);
            }
            
            printf("Parent: Received factorial result: %d\n", shared->number);
        }
        
        // Wait for child to finish
        sleep(1);
        
        // Cleanup shared memory
        shmdt(shared);
        shmctl(shmid, IPC_RMID, NULL);
        
        printf("Parent: Shared memory cleaned up\n");
        
    } else {  // Child process
        while (1) {
            // Wait for parent to write new number
            while (!shared->parent_written) {
                usleep(100000);
            }
            
            int num = shared->number;
            printf("Child: Received number %d\n", num);
            
            // Calculate factorial
            long long fact = factorial(num);
            
            // Write result back
            shared->number = fact;
            shared->parent_written = 0;
            shared->child_written = 1;
            
            printf("Child: Calculated factorial %lld\n", fact);
            
            if (num == 0) break;  // Exit condition
        }
        
        // Detach shared memory
        shmdt(shared);
    }
    
    return 0;
}
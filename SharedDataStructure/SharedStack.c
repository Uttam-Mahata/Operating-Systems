#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

// Stack Element Type (Can be changed to any data type)
typedef int StackElement;

// Stack Structure
typedef struct {
    StackElement *data;      // Array to hold stack elements
    int top;              // Index of the top element (-1 for empty stack)
    int capacity;         // Maximum number of elements the stack can hold
    pthread_mutex_t mutex;   // Mutex for thread-safe access
    pthread_cond_t not_full;  // Condition variable: signal when stack is not full
    pthread_cond_t not_empty; // Condition variable: signal when stack is not empty
} SharedStack;

// Stack Key
#define STACK_KEY 1234

// Function Prototypes
SharedStack* createSharedStack(int capacity);
SharedStack* getSharedStack();
int push(SharedStack* stack, StackElement value);
int pop(SharedStack* stack, StackElement* value);
void destroySharedStack(SharedStack* stack);

// Error Handling Macro
#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

// Create a new shared stack
SharedStack* createSharedStack(int capacity) {
    int shmid;
    SharedStack* stack;

    // Allocate shared memory for the stack structure
    shmid = shmget(STACK_KEY, sizeof(SharedStack), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        return NULL;
    }

    // Attach the shared memory segment
    stack = (SharedStack*) shmat(shmid, NULL, 0);
    if (stack == (SharedStack*) -1) {
        perror("shmat failed");
        return NULL;
    }

    // Allocate memory for the stack data in a different shared memory segment for dynamic sizing
    shmid = shmget(STACK_KEY + 1, sizeof(StackElement) * capacity, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed for stack data");
        shmdt(stack); // Detach previously attached memory
        return NULL;
    }

    stack->data = (StackElement*) shmat(shmid, NULL, 0);
    if (stack->data == (StackElement*) -1) {
        perror("shmat failed for stack data");
        shmdt(stack); // Detach previously attached memory
        return NULL;
    }

    // Initialize the stack
    stack->capacity = capacity;
    stack->top = -1; // Empty Stack

    // Initialize mutex and condition variables (IMPORTANT: PTHREAD_PROCESS_SHARED attribute)
    pthread_mutexattr_t attr;
    pthread_condattr_t cond_attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&stack->mutex, &attr);
    pthread_mutexattr_destroy(&attr); // Clean up the attribute

    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&stack->not_full, &cond_attr);
    pthread_cond_init(&stack->not_empty, &cond_attr);
    pthread_condattr_destroy(&cond_attr); // Clean up the attribute


    printf("Shared stack created with capacity %d\n", capacity);
    return stack;
}


// Get an existing shared stack
SharedStack* getSharedStack() {
    int shmid;
    SharedStack* stack;

    // Get the shared memory segment ID
    shmid = shmget(STACK_KEY, sizeof(SharedStack), 0666);
    if (shmid == -1) {
        perror("shmget failed");
        return NULL;
    }

    // Attach the shared memory segment
    stack = (SharedStack*) shmat(shmid, NULL, 0);
    if (stack == (SharedStack*) -1) {
        perror("shmat failed");
        return NULL;
    }

    return stack;
}

// Push an element onto the stack
int push(SharedStack* stack, StackElement value) {
    int s;
    s = pthread_mutex_lock(&stack->mutex);
    if (s != 0) handle_error_en(s, "pthread_mutex_lock");

    // Wait until the stack is not full
    while (stack->top == stack->capacity - 1) {
        printf("Stack is full.  Waiting...\n");
        s = pthread_cond_wait(&stack->not_full, &stack->mutex);
        if (s != 0) handle_error_en(s, "pthread_cond_wait");
    }

    // Push the element
    stack->top++;
    stack->data[stack->top] = value;
    printf("Pushed %d onto the stack. Top = %d\n", value, stack->top);

    // Signal that the stack is no longer empty
    s = pthread_cond_signal(&stack->not_empty);
    if (s != 0) handle_error_en(s, "pthread_cond_signal");

    s = pthread_mutex_unlock(&stack->mutex);
    if (s != 0) handle_error_en(s, "pthread_mutex_unlock");
    return 0; // Success
}

// Pop an element from the stack
int pop(SharedStack* stack, StackElement* value) {
    int s;
    s = pthread_mutex_lock(&stack->mutex);
    if (s != 0) handle_error_en(s, "pthread_mutex_lock");

    // Wait until the stack is not empty
    while (stack->top == -1) {
        printf("Stack is empty.  Waiting...\n");
        s = pthread_cond_wait(&stack->not_empty, &stack->mutex);
        if (s != 0) handle_error_en(s, "pthread_cond_wait");
    }

    // Pop the element
    *value = stack->data[stack->top];
    stack->top--;
    printf("Popped %d from the stack. Top = %d\n", *value, stack->top);

    // Signal that the stack is no longer full
    s = pthread_cond_signal(&stack->not_full);
    if (s != 0) handle_error_en(s, "pthread_cond_signal");

    s = pthread_mutex_unlock(&stack->mutex);
    if (s != 0) handle_error_en(s, "pthread_mutex_unlock");
    return 0; // Success
}


// Destroy the shared stack
void destroySharedStack(SharedStack* stack) {
    int shmid_stack, shmid_data;

    // Detach from the shared memory segment
    if (shmdt(stack) == -1) {
        perror("shmdt failed for stack struct");
    }
    shmid_stack = shmget(STACK_KEY, sizeof(SharedStack), 0666);
    if (shmid_stack == -1) {
        perror("shmget failed for stack struct");
    }
        if (shmctl(shmid_stack, IPC_RMID, NULL) == -1) {
        perror("shmctl failed for stack struct");
    }


    //Detach from the shared memory segment for stack data
     shmid_data = shmget(STACK_KEY + 1, 0, 0666);
        if (shmid_data == -1) {
        perror("shmget failed for stack data");
        }
    if (shmdt(stack->data) == -1) {
        perror("shmdt failed for stack data");
    }
    if (shmctl(shmid_data, IPC_RMID, NULL) == -1) {
        perror("shmctl failed for stack data");
    }

    printf("Shared stack destroyed\n");
}

// Example Usage (Illustrative)
int main() {
    SharedStack* stack;
    StackElement value;

    // Create a new stack (Only do this once)
   stack = createSharedStack(5);

   if (stack == NULL) {
        fprintf(stderr, "Failed to create shared stack.  Check permissions or resource limits.\n");
        return 1;
   }

    // Get an existing stack (In other processes)
   //stack = getSharedStack();


    // Push some values
    push(stack, 10);
    push(stack, 20);
    push(stack, 30);

    // Pop a value
    pop(stack, &value);
    printf("Popped value: %d\n", value);

    // Destroy the stack (Only do this once, usually when all processes are finished)
   destroySharedStack(stack);

    return 0;
}

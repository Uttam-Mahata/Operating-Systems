#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_TREE_SIZE 20
#define MAX_PRODUCER_THREADS 10
#define MAX_CONSUMER_THREADS 10
#define MAX_SLEEP_TIME 5

// Node structure for binary tree
typedef struct Node {
    int data;
    struct Node *left, *right;
} Node;

// Binary Tree Structure with synchronization
typedef struct {
    Node* root;
    int size;
    int max_size;
    pthread_mutex_t mutex;
    pthread_cond_t empty, full;
} BinaryTree;

// Function prototypes
void initTree(BinaryTree *tree);
Node* createNode(int data);
Node* insert(Node* root, int data, int* success);
void insertTree(BinaryTree *tree, int item);
int deleteMin(BinaryTree *tree);
int isEmpty(BinaryTree *tree);
int isFull(BinaryTree *tree);
void clearResources();
void deleteProducer();
void deleteConsumer();
void freeTree(Node* root);

// Global variables
BinaryTree tree;
pthread_t producerThreads[MAX_PRODUCER_THREADS];
pthread_t consumerThreads[MAX_CONSUMER_THREADS];
pthread_t managerThread;
int numProducers = 0, numConsumers = 0;

// Initialize the tree
void initTree(BinaryTree *tree) {
    tree->root = NULL;
    tree->size = 0;
    tree->max_size = MAX_TREE_SIZE;
    pthread_mutex_init(&tree->mutex, NULL);
    pthread_cond_init(&tree->empty, NULL);
    pthread_cond_init(&tree->full, NULL);
}

// Create a new node
Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode) {
        newNode->data = data;
        newNode->left = newNode->right = NULL;
    }
    return newNode;
}

// Insert a node into the tree recursively
Node* insert(Node* root, int data, int* success) {
    if (root == NULL) {
        *success = 1;
        return createNode(data);
    }

    if (data < root->data)
        root->left = insert(root->left, data, success);
    else if (data > root->data)
        root->right = insert(root->right, data, success);
    else
        *success = 0; // Duplicate value, don't increase size

    return root;
}

// Thread-safe insert into tree
void insertTree(BinaryTree *tree, int item) {
    fprintf(stderr, "\t\t\t\tCalled insertTree!\n");
    fprintf(stderr, "\t\t\t\tinsertTree locking!\n");
    pthread_mutex_lock(&tree->mutex);
    
    while (isFull(tree)) {
        fprintf(stderr, "\t\t\t\tinsertTree waiting!\n");
        pthread_cond_wait(&tree->full, &tree->mutex);
    }
    
    int success = 0;
    tree->root = insert(tree->root, item, &success);
    if (success)
        tree->size++;
    
    fprintf(stderr, "\t\t\t\tinsertTree signalling!\n");
    pthread_cond_signal(&tree->empty);
    fprintf(stderr, "\t\t\t\tinsertTree unlocking!\n");
    pthread_mutex_unlock(&tree->mutex);
}

// Find and remove the minimum value node
int deleteMin(BinaryTree *tree) {
    fprintf(stderr, "\t\t\t\t\t\tCalled deleteMin!\n");
    fprintf(stderr, "\t\t\t\t\t\tdeleteMin locking!\n");
    pthread_mutex_lock(&tree->mutex);
    
    while (isEmpty(tree)) {
        fprintf(stderr, "\t\t\t\t\t\tdeleteMin waiting!\n");
        pthread_cond_wait(&tree->empty, &tree->mutex);
    }
    
    // Find the minimum value (leftmost node)
    Node** current = &tree->root;
    Node* parent = NULL;
    
    while ((*current)->left != NULL) {
        parent = *current;
        current = &((*current)->left);
    }
    
    int minVal = (*current)->data;
    Node* temp = *current;
    
    // If the node has a right child
    if ((*current)->right != NULL) {
        *current = (*current)->right;
    } else {
        // No right child
        if (parent == NULL) {
            // Root case
            tree->root = tree->root->right;
        } else {
            parent->left = NULL;
        }
    }
    
    free(temp);
    tree->size--;
    
    fprintf(stderr, "\t\t\t\t\t\tdeleteMin signalling!\n");
    pthread_cond_signal(&tree->full);
    fprintf(stderr, "\t\t\t\t\t\tdeleteMin unlocking!\n");
    pthread_mutex_unlock(&tree->mutex);
    
    return minVal;
}

// Check if tree is empty
int isEmpty(BinaryTree *tree) {
    return tree->size == 0;
}

// Check if tree is full
int isFull(BinaryTree *tree) {
    return tree->size >= tree->max_size;
}

// Free all nodes in the tree
void freeTree(Node* root) {
    if (root) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// Producer function
void *producer(void *data) {
    int producerId = *((int *)data);
    while (1) {
        srand(time(NULL) + producerId);
        int numItems = rand() % (MAX_TREE_SIZE / 2) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = rand() % 1000;
            insertTree(&tree, item);
            printf("\t\t\t\tProducer %d inserted %d/%d item: %d\n", 
                   producerId, i+1, numItems, item);
        }
        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
    return NULL;
}

// Consumer function
void *consumer(void *data) {
    int consumerId = *((int *)data);
    while (1) {
        srand(time(NULL) + consumerId);
        int numItems = rand() % (MAX_TREE_SIZE / 2) + 1;
        printf("\n");
        for (int i = 0; i < numItems; ++i) {
            int item = deleteMin(&tree);
            printf("\t\t\t\t\t\tConsumer %d removed %d/%d min item: %d\n", 
                   consumerId, i+1, numItems, item);
        }
        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
    return NULL;
}

// Clear resources function
void clearResources() {
    // Clear producer threads
    for (int i = 0; i < numProducers; ++i) {
        pthread_cancel(producerThreads[i]);
    }
    // Clear consumer threads
    for (int i = 0; i < numConsumers; ++i) {
        pthread_cancel(consumerThreads[i]);
    }
    // Destroy mutex and condition variables
    pthread_mutex_destroy(&tree.mutex);
    pthread_cond_destroy(&tree.empty);
    pthread_cond_destroy(&tree.full);
    // Free tree nodes
    freeTree(tree.root);
    tree.root = NULL;
    tree.size = 0;
    printf("All threads and resources cleared.\n");
    // Reset thread counts
    numProducers = 0;
    numConsumers = 0;
}

// Delete producer function
void deleteProducer() {
    if (numProducers > 0) {
        pthread_cancel(producerThreads[numProducers - 1]);
        printf("Producer thread %d deleted.\n", numProducers);
        numProducers--;
    } else {
        printf("No producer threads to delete.\n");
    }
}

// Delete consumer function
void deleteConsumer() {
    if (numConsumers > 0) {
        pthread_cancel(consumerThreads[numConsumers - 1]);
        printf("Consumer thread %d deleted.\n", numConsumers);
        numConsumers--;
    } else {
        printf("No consumer threads to delete.\n");
    }
}

// Manager thread function
void *manager(void *data) {
    char choice;
    printf("Welcome to the Binary Tree manager thread!\n");
    do {
        printf("\nMenu:\n");
        printf("1. Add Producer\n");
        printf("2. Add Consumer\n");
        printf("3. Delete Producer\n");
        printf("4. Delete Consumer\n");
        printf("5. Clear All Threads, Resources and Exit\n");
        printf("Enter your choice: ");
        scanf(" %c", &choice);

        switch (choice) {
            case '1':
                if (numProducers < MAX_PRODUCER_THREADS) {
                    pthread_create(&producerThreads[numProducers], NULL, 
                                 producer, &numProducers);
                    printf("Added producer thread %d\n", numProducers + 1);
                    numProducers++;
                } else {
                    printf("Cannot add more producer threads.\n");
                }
                break;
            case '2':
                if (numConsumers < MAX_CONSUMER_THREADS) {
                    pthread_create(&consumerThreads[numConsumers], NULL, 
                                 consumer, &numConsumers);
                    printf("Added consumer thread %d\n", numConsumers + 1);
                    numConsumers++;
                } else {
                    printf("Cannot add more consumer threads.\n");
                }
                break;
            case '3':
                deleteProducer();
                break;
            case '4':
                deleteConsumer();
                break;
            case '5':
                clearResources();
                printf("Exiting manager thread.\n");
                break;
            default:
                printf("Invalid choice!\n");
        }
    } while (choice != '5');

    return NULL;
}

int main() {
    initTree(&tree);
    pthread_create(&managerThread, NULL, manager, NULL);
    pthread_join(managerThread, NULL); // Wait for manager thread to finish
    return 0;
}

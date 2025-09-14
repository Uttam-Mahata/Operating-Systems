# Client-Server Application with Shared Memory

This directory contains a simple client-server application that demonstrates inter-process communication (IPC) using shared memory. The application consists of two programs: a server and a worker.

## Programs Overview

### 1. Server (`server.c`)
- The server creates a shared memory segment.
- It generates random strings and places them in the shared memory.
- It then waits for a worker process to process the string and put the result back in the shared memory.
- The server prints the processed string and continues with the next random string.

### 2. Worker (`worker.c`)
- The worker attaches to the shared memory segment created by the server.
- It waits for the server to put a string in the shared memory.
- It processes the string (in this case, by converting it to uppercase).
- It then puts the processed string back in the shared memory for the server to retrieve.

## How to Compile and Run

1.  **Compile the programs:**

    ```bash
    gcc server.c -o server.out
    gcc worker.c -o worker.out
    ```

2.  **Run the server in one terminal:**

    ```bash
    ./server.out
    ```

3.  **Run the worker in another terminal:**

    ```bash
    ./worker.out
    ```

You will see the server generating random strings, and the worker processing them and sending them back to the server.

## Shared Memory and Synchronization

The server and worker processes use a shared memory segment to exchange data. They use a `status` flag within the shared memory structure for synchronization:

-   `status = 0`: The shared memory is idle, and the server can place a new task.
-   `status = 1`: A new task is available for the worker.
-   `status = 2`: The worker has taken the task and is processing it.
-   `status = 3`: The worker has completed the task, and the result is available.
-   `status = 4`: The server has consumed the result, and the shared memory is idle again.
-   `status = -1`: The server is terminating, and the worker should also terminate.

This simple synchronization mechanism ensures that the server and worker do not access the shared memory at the same time.

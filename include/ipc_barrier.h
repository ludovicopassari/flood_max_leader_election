#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define IPC_BARRIER_FIFO "./tmp/fifo/ipc_barrier_sync.fifo"

typedef struct {
    char fifo_name[256];
    char response_fifo[256];  // FIFO per la risposta
    int count; // Number of processes waiting at the barrier at current time
    int total; // Total number of processes expected to reach the barrier
    pid_t coordinator_pid;  // PID del processo coordinatore
} ipc_barrier_t;

typedef struct {
    int pid;
    char text[64];
} message_t;

void init_ipc_barrier(ipc_barrier_t *barrier, char* barrier_name, int total);
void destroy_ipc_barrier(ipc_barrier_t *barrier);

void wait_ipc_barrier(ipc_barrier_t *barrier);
void signal_ipc_barrier(ipc_barrier_t *barrier);


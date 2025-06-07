#include "ipc_barrier.h"
#include "utils.h"

#define GRAPH_ORDER 6

ipc_barrier_t *barrier;

void processor_task(int id) {
    int pid = getpid();
    printf("Processor %d started.\n", pid);
    
    srand(time(NULL) ^ (pid << 16));
    usleep(rand() % 3000 * 1000); 

    wait_ipc_barrier(barrier); // Wait for the barrier

    srand(time(NULL) ^ (pid << 16));
    usleep(rand() % 3000 * 1000); 

    wait_ipc_barrier(barrier); // Wait for the coordinator to signal

    printf("Processor %d finished.\n", pid);
}

int main(){

    barrier = malloc(sizeof(ipc_barrier_t));

    init_ipc_barrier(barrier, "sync_barrier", GRAPH_ORDER);

    pid_t pids[GRAPH_ORDER];
    
    for (int i = 0; i < GRAPH_ORDER; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("Fork failed");
            return 1;
        } else if (pids[i] == 0) { 
            processor_task(i+1);
            exit(0); 
        }
    }


    wait_and_signal_ipc_barrier(barrier); // Coordinator waits for all processes to reach the barrier
    
    reset_ipc_barrier(barrier); // Reset the barrier for the next phase

    wait_and_signal_ipc_barrier(barrier); // Coordinator signals all processes to continue
    
    for (int i = 0; i < GRAPH_ORDER; i++) waitpid(pids[i], NULL, 0); 
    

    destroy_ipc_barrier(barrier); // Clean up the barrier


    return 0;
}
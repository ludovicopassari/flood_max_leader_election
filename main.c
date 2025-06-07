#include "ipc_barrier.h"
#include "utils.h"

#include <time.h>


#define GRAPH_ORDER 6
ipc_barrier_t *send_barrier;

void processor_task(int id) {
    printf("Processor %d started.\n", id);
        // Inizializza il seed del random solo nel figlio
    srand(time(NULL) ^ (getpid() << 16));

    int delay_ms = rand() % 3000; // Delay casuale tra 0 e 2999 ms

    usleep(delay_ms * 1000); // Converti in microsecondi

    wait_ipc_barrier(send_barrier); // Wait for the barrier
    // Simulate some processing
    sleep(3);
    
    printf("Processor %d finished.\n", id);
}

int main(){

    send_barrier = malloc(sizeof(ipc_barrier_t));

    init_ipc_barrier(send_barrier, "barrier_sync", GRAPH_ORDER);

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

    sleep(2); // Ensure all child processes are ready before proceeding

    
    //open the barrier FIFO for reading
    printf("Main process waiting for all processors to reach the barrier...\n");
    int fd_req, fd_resp;
    int arrived = 0;
    char buffer[256];

    // Apri FIFO di richiesta in lettura
    fd_req = open("./tmp/barrier_sync_req", O_RDONLY);
    if (fd_req < 0) {
        perror("Coordinator: Failed to open request FIFO");
        exit(EXIT_FAILURE);
    }

    message_t msg_req;
    msg_req.pid = getpid();
    // Conta i processi che arrivano
    while (arrived < send_barrier->total) {
        
        ssize_t bytes_read = read(fd_req, &msg_req, sizeof(message_t));
        if (bytes_read > 0) {
            arrived++;
            printf("Coordinator: Process %d/%d arrived at barrier. Message :%s, PID: %d \n", arrived, send_barrier->total, msg_req.text, msg_req.pid);
        }
    }

    close(fd_req);

  
    
    printf("Coordinator: All processes arrived, releasing barrier\n");

    // Apri FIFO di risposta in scrittura
    fd_resp = open("./tmp/barrier_sync_resp", O_WRONLY);
    if (fd_resp < 0) {
        perror("Coordinator: Failed to open response FIFO");
        exit(EXIT_FAILURE);
    }
    
    message_t release_msg;
    release_msg.pid = getpid();
    // Invia segnale di sblocco a tutti i processi
    for (int i = 0; i < send_barrier->total; i++) {
        if (write(fd_resp, &release_msg, sizeof(message_t)) < 0) {
            perror("Coordinator: Failed to write release signal");
            break;
        }
    }
    
    close(fd_resp);




    for (int i = 0; i < GRAPH_ORDER; i++) {
        waitpid(pids[i], NULL, 0); // Wait for each child process to finish
    }

    destroy_ipc_barrier(send_barrier); // Clean up the barrier


    return 0;
}
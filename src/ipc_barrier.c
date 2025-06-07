#include "ipc_barrier.h"
#include "utils.h"

void init_ipc_barrier(ipc_barrier_t *barrier, char* barrier_name, int total) {
    if (barrier == NULL || barrier_name == NULL) {
        fprintf(stderr, "Error: Invalid barrier or barrier_name\n");
        exit(EXIT_FAILURE);
    }
    
    if (total <= 0) {
        fprintf(stderr, "Error: Total number of processes must be greater than 0.\n");
        exit(EXIT_FAILURE);
    }

    // Inizializza la struttura barriera
    snprintf(barrier->fifo_name, sizeof(barrier->fifo_name), "./tmp/%s_req", barrier_name);
    snprintf(barrier->response_fifo, sizeof(barrier->response_fifo), "./tmp/%s_resp", barrier_name);
    barrier->count = 0;
    barrier->total = total;
    barrier->coordinator_pid = getpid();

    // Crea le FIFO per richieste e risposte
    if (create_fifo(barrier->fifo_name) < 0) {
        fprintf(stderr, "Failed to create request FIFO: %s\n", barrier->fifo_name);
        exit(EXIT_FAILURE);
    }
    
    if (create_fifo(barrier->response_fifo) < 0) {
        fprintf(stderr, "Failed to create response FIFO: %s\n", barrier->response_fifo);
        exit(EXIT_FAILURE);
    }

    #ifdef DEBUG
    printf("IPC Barrier initialized: %s (total: %d)\n", barrier_name, total);
    printf("Request FIFO: %s\n", barrier->fifo_name);
    printf("Response FIFO: %s\n", barrier->response_fifo);
    #endif
}


void wait_ipc_barrier(ipc_barrier_t *barrier) {

    int fd_req, fd_resp;
    int current_pid = getpid();
    char buffer[256];

   // Apri FIFO di richiesta in scrittura
    fd_req = open("./tmp/barrier_sync_req", O_WRONLY);
    if (fd_req < 0) {
        perror("Failed to open request FIFO for writing");
        exit(EXIT_FAILURE);
    }
    
    message_t msg_req;
    msg_req.pid = current_pid;
    snprintf(msg_req.text, sizeof(msg_req.text), "PROC_%d", current_pid);

    if (write(fd_req, &msg_req, sizeof(message_t)) < 0) {
        perror("Failed to write to request FIFO");
        close(fd_req);
        exit(EXIT_FAILURE);
    }

    close(fd_req);

    // Apri FIFO di risposta in lettura
    fd_resp = open("./tmp/barrier_sync_resp", O_RDONLY);
    if (fd_resp < 0) {
        perror("Failed to open response FIFO for reading");
        exit(EXIT_FAILURE);
    }
    message_t msg_resp;
    
    // Attendi segnale di sblocco
    if (read(fd_resp, &msg_resp, sizeof(message_t)) < 0) {
        perror("Failed to read from response FIFO");
        close(fd_resp);
        exit(EXIT_FAILURE);
    }
    printf("Process %d received response: %d\n", current_pid, msg_resp.pid);
    close(fd_resp);

}

void destroy_ipc_barrier(ipc_barrier_t *barrier) {
    if (barrier != NULL) {
        remove_existing_fifo(barrier->fifo_name); // Remove the FIFO if it exists
        free(barrier); // Free the barrier structure
    }
}
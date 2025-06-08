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

    snprintf(barrier->fifo_name, sizeof(barrier->fifo_name), "./tmp/%s_req", barrier_name);
    snprintf(barrier->response_fifo, sizeof(barrier->response_fifo), "./tmp/%s_resp", barrier_name);

    printf("Initializing IPC barrier with FIFO: %s and response FIFO: %s\n", 
           barrier->fifo_name, barrier->response_fifo);
    barrier->count = 0;
    barrier->total = total;
    barrier->coordinator_pid = getpid();

    if (create_fifo(barrier->fifo_name) < 0) {
        fprintf(stderr, "Failed to create request FIFO: %s\n", barrier->fifo_name);
        exit(EXIT_FAILURE);
    }
    
    if (create_fifo(barrier->response_fifo) < 0) {
        fprintf(stderr, "Failed to create response FIFO: %s\n", barrier->response_fifo);
        exit(EXIT_FAILURE);
    }
}

void wait_ipc_barrier(ipc_barrier_t *barrier, message_t *msg_rel) {
    int fd_req, fd_resp;
    int current_pid = getpid();

    fd_req = open(barrier->fifo_name, O_WRONLY);
    if (fd_req < 0) {
        perror("Failed to open request FIFO for writing");
        exit(EXIT_FAILURE);
    }

    message_t msg_req;
    msg_req.pid = current_pid;
    snprintf(msg_req.text, sizeof(msg_req.text), "PROC_%d", current_pid);

    srand(time(NULL) ^ (getpid() << 16));
    usleep(rand() % 3000 * 1000); // Converti in microsecondi

    if (write(fd_req, &msg_req, sizeof(message_t)) < 0) {
        perror("Failed to write to request FIFO");
        close(fd_req);
        exit(EXIT_FAILURE);
    }

    //close(fd_req);


    fd_resp = open(barrier->response_fifo, O_RDONLY);
    if (fd_resp < 0) {
        perror("Failed to open response FIFO for reading");
        exit(EXIT_FAILURE);
    }

    if(msg_rel == NULL) {
        msg_rel = malloc(sizeof(message_t));
        if (msg_rel == NULL) {
            perror("Failed to allocate memory for release message");
            close(fd_resp);
            exit(EXIT_FAILURE);
        }
    }
    memset(msg_rel, 0, sizeof(message_t)); // Inizializza il messaggio di rilascio
    // Attendi segnale di sblocco
    if (read(fd_resp, msg_rel, sizeof(message_t)) < 0) {
        perror("Failed to read from response FIFO");
        close(fd_resp);
        exit(EXIT_FAILURE);
    }
    //close(fd_resp);

}

void destroy_ipc_barrier(ipc_barrier_t *barrier) {

    if (barrier != NULL) {
        remove_existing_fifo(barrier->fifo_name); // Remove the FIFO if it exists
        remove_existing_fifo(barrier->response_fifo); // Remove the response FIFO if it exists
        free(barrier); // Free the barrier structure
    }
}

void wait_and_signal_ipc_barrier(ipc_barrier_t *barrier, message_t *release_msg) {
    int fd_req, fd_resp;
    int arrived = 0;

    // Apri FIFO di richiesta in lettura
    fd_req = open(barrier->fifo_name, O_RDONLY);
    if (fd_req < 0) {
        perror("Coordinator: Failed to open request FIFO");
        exit(EXIT_FAILURE);
    }

    message_t msg_req;
    msg_req.pid = getpid();

    while (arrived < barrier->total) {
        ssize_t bytes_read = read(fd_req, &msg_req, sizeof(message_t));
        if (bytes_read > 0) {
            arrived++;
        }
    }
    //close(fd_req);


    // Apri FIFO di risposta in scrittura
    fd_resp = open(barrier->response_fifo, O_WRONLY);
    if (fd_resp < 0) {
        perror("Coordinator: Failed to open response FIFO");
        exit(EXIT_FAILURE);
    }
    
    if (release_msg == NULL) {
        release_msg = malloc(sizeof(message_t));
        if (release_msg == NULL) {
            perror("Coordinator: Failed to allocate memory for release message");
            close(fd_resp);
            exit(EXIT_FAILURE);
        }
    }

    release_msg->pid = getpid();
    // Invia segnale di sblocco a tutti i processi
    for (int i = 0; i < barrier->total; i++) {
        if (write(fd_resp, release_msg, sizeof(message_t)) < 0) {
            perror("Coordinator: Failed to write release signal");
            break;
        }
    }
    
    usleep(500); // Converti in microsecondi

    //close(fd_resp);

}

void reset_ipc_barrier(ipc_barrier_t *barrier) {
    if (barrier != NULL) {
        barrier->count = 0; // Reset the count for the next use
    }
}


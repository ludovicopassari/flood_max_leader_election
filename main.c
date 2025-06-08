#include "ipc_barrier.h"
#include "utils.h"
#include "sync_network.h"

ipc_barrier_t *barrier;

void* processor_task(processor_state_t* w) {

    int pid = getpid();
    //printf("Processor %d started.\n", pid);

    int* out_neighbors = NULL;
    int* in_neighbors = NULL;

    int out_neighbors_count = 0;
    int in_neighbors_count = 0;

    // get out neighbors and in neighbors
    get_neighbors(w->my_id, &out_neighbors, &in_neighbors, &out_neighbors_count, &in_neighbors_count);

    //init edges
    for (int i = 0; i < out_neighbors_count; i++){
        
        w->out_edges[i].source_id = w->my_id;
        w->out_edges[i].target_id = out_neighbors[i];
        snprintf(w->out_edges[i].fifo_path, sizeof(w->out_edges[i].fifo_path), "%s/%d_to_%d.fifo", BASE_FIFO_DIR, w->my_id, out_neighbors[i]);

    }

    for (int i = 0; i < in_neighbors_count; i++){
        
        w->in_edges[i].source_id = in_neighbors[i];
        w->in_edges[i].target_id = w->my_id;
        snprintf(w->in_edges[i].fifo_path, sizeof(w->in_edges[i].fifo_path), "%s/%d_to_%d.fifo", BASE_FIFO_DIR, in_neighbors[i], w->my_id);
    }


    // open to read FIFOs for in neighbors
    for (int i = 0; i < in_neighbors_count; i++) {
        //char fifo_name[256];
        //snprintf(fifo_name, sizeof(fifo_name), "%s/%d_to_%d.fifo", BASE_FIFO_DIR, in_neighbors[i], w->my_id);
        w->in_edges[i].fd = open(w->in_edges[i].fifo_path, O_RDONLY | O_NONBLOCK);
        if (w->in_edges[i].fd < 0) {
            perror("Failed to open FIFO for reading");
        }
    }

    usleep(1000);

    // open to write FIFOs for out neighbors
    for (int i = 0; i < out_neighbors_count; i++) {
        //char fifo_name[256];
        //snprintf(fifo_name, sizeof(fifo_name), "%s/%d_to_%d.fifo",BASE_FIFO_DIR ,w->my_id, out_neighbors[i]);
        w->out_edges[i].fd = open(w->out_edges[i].fifo_path, O_WRONLY | O_NONBLOCK);
        if (w->out_edges[i].fd < 0) {
            perror("Failed to open FIFO for writing");
        }
    }

    //message_t start_condition_msg;
    //wait_ipc_barrier(barrier,&start_condition_msg); // Wait for the barrier to ensure coordinator that all worker have fifos opened.
    //w->round = atoi(start_condition_msg.text);  

    while (1)
    {   
        // print massege to describe fase
        //printf("Processor %d is wating for round number\n", w->my_id);
        message_t start_condition_msg;
        wait_ipc_barrier(barrier,&start_condition_msg); // Wait for the barrier to ensure coordinator that all worker have fifos opened.
        w->round = atoi(start_condition_msg.text);

        printf("State of processor %d : my_id: %d, max_id: %d, leader: %d, round: %d\n", w->my_id, w->my_id, w->max_id, w->leader, w->round);

        // send messages to out neighbors
        for (int i = 0; i < out_neighbors_count; i++) {
            int *msg_snd = msg(w, i); // get message to send
            if (write(w->out_edges[i].fd, msg_snd, sizeof(msg)) < 0) {
                perror("Failed to write to FIFO");
            }

        }

        //printf("Processor %d is wating for all processor writes\n", w->my_id);
        wait_ipc_barrier(barrier, NULL); // wait for the barrier to ensure all workers have sent their messages

        // receive messages from in neighbors
        int rcv_tokens[GRAPH_ORDER];
        int token_count = 0;
        for (int i = 0; i < in_neighbors_count; i++) {
            int msg;
            ssize_t bytes_read = read(w->in_edges[i].fd, &msg, sizeof(msg));
            if (bytes_read > 0) {
                rcv_tokens[token_count++] = msg; // store received token
            } else if (bytes_read < 0) {
                perror("Failed to read from FIFO");
            }
        }


        //printf("Processor %d is wating for all processor reads\n", w->my_id);
        wait_ipc_barrier(barrier,NULL); // wait for the barrier to ensure all workers have recived their messages

        processor_state_t *new_w = (processor_state_t*) stf(w, rcv_tokens , token_count);
        w= new_w; // update processor state with new state
    }
    
    printf("Processor %d finished.\n", pid);

    return NULL; 
}   

int main(){

    processor_state_t* processors[GRAPH_ORDER];
    // compute the graph diameter
    graph_diam = compute_diameter();
    printf("Graph diameter : %d \n", graph_diam);

    // create processor states
    for(int i = 0; i < 6; i++) {
        processors[i] = malloc(sizeof(processor_state_t));
        if(processors[i] == NULL) {
            perror("Failed to allocate memory for processor_state_t");
            return 1; 
        }
        // Initialize processor state
        init_processor(processors[i], i + 1 );
    }

    create_fifos();


    barrier = malloc(sizeof(ipc_barrier_t));
    init_ipc_barrier(barrier, "sync_barrier", GRAPH_ORDER);

    pid_t pids[GRAPH_ORDER];
    
    for (int i = 0; i < GRAPH_ORDER; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("Fork failed");
            return 1;
        } else if (pids[i] == 0) { 
            processor_task(processors[i]);
            exit(0); 
        }
    }

    int round = 0;

    message_t release_msg;
    while (round < 6) {
        printf("\n");
        memset(release_msg.text, 0, sizeof(release_msg.text)); // Clear the buffer
        sprintf(release_msg.text, "%d", round++);
        wait_and_signal_ipc_barrier(barrier, &release_msg);
        
        
        reset_ipc_barrier(barrier);

        wait_and_signal_ipc_barrier(barrier,NULL); // Coordinator waits for all processes to send their messages

        reset_ipc_barrier(barrier);

        wait_and_signal_ipc_barrier(barrier,NULL);
        
        reset_ipc_barrier(barrier); 
        
        printf("===================================================================\n");
    }

    for (int i = 0; i < GRAPH_ORDER; i++) waitpid(pids[i], NULL, 0); 
    

    destroy_ipc_barrier(barrier); // Clean up the barrier

    if (remove_all_fifos() < 0) {
        perror("Failed to remove all FIFOs");
        return 1;
    }
    return 0;
}
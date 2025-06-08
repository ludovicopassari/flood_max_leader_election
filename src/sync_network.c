#include "sync_network.h"
#include "utils.h"


int adj_matrix[GRAPH_ORDER][GRAPH_ORDER] = {
    {0, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 0},
    {0, 0, 0, 1, 1, 0},
    {0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 1},
    {0, 0, 0, 1, 1, 0}
};

// Function to get neighbors of a node
void get_neighbors(int my_id, int** out_neighbors, int** in_neighbors, int* out_count, int* in_count) {
    // Validazione input
    if (my_id < 1 || my_id > GRAPH_ORDER || !out_neighbors || !in_neighbors || !out_count || !in_count) {
        if (out_count) *out_count = 0;
        if (in_count) *in_count = 0;
        return;
    }
    
    int node_idx = my_id - 1; // Indice 0-based del nodo
    int out_cnt = 0, in_cnt = 0;
    
    // Singolo passaggio per contare sia out che in neighbors
    for (int i = 0; i < GRAPH_ORDER; i++) {
        if (adj_matrix[node_idx][i] == 1) out_cnt++;
        if (adj_matrix[i][node_idx] == 1) in_cnt++;
    }
    
    // Allocazione memoria
    *out_neighbors = (out_cnt > 0) ? malloc(out_cnt * sizeof(int)) : NULL;
    *in_neighbors = (in_cnt > 0) ? malloc(in_cnt * sizeof(int)) : NULL;
    
    // Controllo allocazioni
    if (out_cnt > 0 && *out_neighbors == NULL) {
        perror("Failed to allocate memory for out_neighbors");
        *out_count = *in_count = 0;
        return;
    }
    
    if (in_cnt > 0 && *in_neighbors == NULL) {
        perror("Failed to allocate memory for in_neighbors");
        free(*out_neighbors);
        *out_neighbors = NULL;
        *out_count = *in_count = 0;
        return;
    }
    
    // Riempimento array in un singolo passaggio
    int out_idx = 0, in_idx = 0;
    for (int i = 0; i < GRAPH_ORDER; i++) {
        if (adj_matrix[node_idx][i] == 1) {
            (*out_neighbors)[out_idx++] = i + 1;
        }
        if (adj_matrix[i][node_idx] == 1) {
            (*in_neighbors)[in_idx++] = i + 1;
        }
    }
    
    *out_count = out_cnt;
    *in_count = in_cnt;
}

// Function to create FIFOs for communication between processors
void create_fifos(){
    char fifo_name[256];
    // compute fifo names and create fifos
    for (int i = 0; i < GRAPH_ORDER; i++) {
        for (int j = 0; j < GRAPH_ORDER; j++) {
            if (adj_matrix[i][j] == 1) {
                // Create FIFO for outgoing messages
                snprintf(fifo_name, sizeof(fifo_name), "%s/%d_to_%d.fifo", BASE_FIFO_DIR, i + 1, j + 1);
                if(access(fifo_name, F_OK) == 0) {
                    // FIFO already exists
                    if (unlink(fifo_name) == 0){
                        #ifdef DEBUG
                        printf("Removed existing FIFO: %s\n", fifo_name);
                        #endif
                    } else {
                        perror("Failed to remove existing FIFO");
                        return ;
                    }
                }
                if (mkfifo(fifo_name, 0666) < 0) {
                    perror("Failed to create FIFO");
                    return ;
                }
            }
        }
    }
}

void init_processor(processor_state_t* w, int id) {
    w->my_id = id;
    w->max_id = id;
    w->leader = 0; // initially not a leader
    w->round = 0; // start at round 0
}

// Function to remove all FIFOs based on the adjacency matrix
int remove_all_fifos() {
    char fifo_name[256];
    for (int i = 0; i < GRAPH_ORDER; i++) {
        for (int j = 0; j < GRAPH_ORDER; j++) {
            if (adj_matrix[i][j] == 1) {
                snprintf(fifo_name, sizeof(fifo_name), "%s/%d_to_%d.fifo", BASE_FIFO_DIR, i + 1, j + 1);
                if (remove_existing_fifo(fifo_name) < 0) {
                    return -1;
                }
            }
        }
    }
    return 0;
}

// Function to find the maximum ID from an array of IDs
int find_max(int* i, int ids_count) {
    int max = i[0];
    for(int j = 1; j < ids_count; j++) {
        if(i[j] > max) {
            max = i[j];
        }
    }
    return max;
}

// Function to perform BFS and find the maximum distance from the start node
int bfs(int start) {
    int visited[GRAPH_ORDER] = {0};
    int distance[GRAPH_ORDER];
    for (int i = 0; i < GRAPH_ORDER; i++) distance[i] = INF;

    int queue[GRAPH_ORDER];
    int front = 0, rear = 0;

    queue[rear++] = start;
    visited[start] = 1;
    distance[start] = 0;

    while (front < rear) {
        int curr = queue[front++];
        for (int i = 0; i < GRAPH_ORDER; i++) {
            if (adj_matrix[curr][i] && !visited[i]) {
                visited[i] = 1;
                distance[i] = distance[curr] + 1;
                queue[rear++] = i;
            }
        }
    }

    int max_dist = 0;

    for (int i = 0; i < GRAPH_ORDER; i++) {
        if (distance[i] != INF && distance[i] > max_dist) {
            max_dist = distance[i];
        }
    }

    return max_dist;
}

// Function to compute the diameter of the graph using BFS
int compute_diameter() {
    int max_diameter = 0;
    for (int i = 0; i < GRAPH_ORDER; i++) {
        int d = bfs(i);
        if (d > max_diameter) {
            max_diameter = d;
        }
    }
    return max_diameter;
}

/* // Function to send a message to the processor
int* msg(processor_state_t* w, int i) {
    int *res = malloc(sizeof(int));
    *res = 0;
    if(w->round < graph_diam) {
        *res = w->max_id;
        return res;
    }
    return res;
} */

int* msg(processor_state_t* w, int i) {
    int *res = malloc(sizeof(int));
    if (w->round <= graph_diam) { // <= invece di <
        *res = w->max_id;
    } else {
        *res = 0; // oppure potresti anche non mandare nulla dopo
    }
    return res;
}

// Function to process received tokens and determine the new state
void* stf(processor_state_t* w, int* rcv_tokens , int token_count) {
    processor_state_t* new_state = malloc(sizeof(processor_state_t));
    if (new_state == NULL) {
        perror("Failed to allocate memory for processor_state_t");
        return NULL; 
    }
    memcpy(new_state, w, sizeof(processor_state_t)); // copia contenuto di w

    new_state->my_id = w->my_id;
    new_state->max_id = find_max(rcv_tokens, token_count); 
    if(w->round < graph_diam){
        new_state->leader = 0; // not a leader yet
    }else if(w->round == graph_diam && new_state->max_id == w->my_id) {
        new_state->leader = 1; // this processor is the leader
    }else if(w->round == graph_diam && new_state->max_id > w->my_id) {
        new_state->leader = 0; // this processor is not the leader
    }
      
    return new_state;
}



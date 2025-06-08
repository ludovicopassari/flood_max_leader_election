#ifndef SYNC_NETWORK_H
#define SYNC_NETWORK_H

#define BASE_FIFO_DIR "./tmp"
#define GRAPH_ORDER 6
#define INF 1000000 

int graph_diam;

int adj_matrix[GRAPH_ORDER][GRAPH_ORDER];

typedef struct {
    int source_id;      // ID del nodo di origine
    int target_id;      // ID del nodo di destinazione
    char fifo_path[32]; // Percorso della FIFO
    int fd;            // File descriptor quando la FIFO è aperta
} edge_fifo_t;


typedef struct {
    int my_id;
    int max_id;
    int leader; // bolean variable
    int round;
    edge_fifo_t out_edges[10];
    edge_fifo_t in_edges[10];
}processor_state_t;



void get_neighbors(int my_id, int** out_neighbors, int** in_neighbors, int* out_count, int* in_count);

void create_fifos();
void init_processor(processor_state_t* w, int id);
int remove_all_fifos();
int find_max(int* i, int ids_count);
int bfs(int start);
int compute_diameter();
int* msg(processor_state_t* w, int i);
void* stf(processor_state_t* w, int* rcv_tokens , int token_count);

#endif 

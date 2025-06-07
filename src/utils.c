#include "utils.h"

int create_fifo(char* fifo_name) {
    printf("Creating FIFO: %s\n", fifo_name);
    // If the FIFO already exists, remove it
    if (remove_existing_fifo(fifo_name) < 0) {
        perror("Failed to remove existing FIFO");
        return -1;
    }

    // Create the FIFO
    if (mkfifo(fifo_name, 0666) < 0) {
        perror("Failed to create FIFO");
        return 0;
    }

    return 1; // Success
}

int remove_existing_fifo(const char* fifo_name) {
    if (access(fifo_name, F_OK) == 0) {
        if (unlink(fifo_name) == 0) {
            #ifdef DEBUG
            printf("Removed existing FIFO: %s\n", fifo_name);
            #endif
            return 0;
        } else {
            perror("Failed to remove existing FIFO");
            return -1;
        }
    }
    return 0;
}

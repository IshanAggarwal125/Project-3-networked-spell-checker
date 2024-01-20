// server.h

//#ifndef SERVER_H
#define SERVER_H

// Define your connection cell structure
struct connectionCell {
    int socketDescriptor;
    time_t connectionTime;
    int priority;
};

// Declare the global connection queue and its size
// extern struct connectionCell* connection_queue;
// extern int connection_queue_size;

// Function to initialize the connection queue
// void initializeConnectionQueue(int numberOfCells);


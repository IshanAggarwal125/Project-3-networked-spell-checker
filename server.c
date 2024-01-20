#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include "server.h"

#define DEAULT_DICTIONARY "dictionary.txt"
#define DEFAULT_PORT_NUMBER 2700
#define MAX_NUMBER_OF_WORDS 100
#define QUEUE_SIZE 10


//void initializeNetwork(int portNumber);
void *workerThreadFunc(void *args);
void *acceptClientThreadsFunc(void *args);
char **loadDictionary(char *dictionaryFile);
struct connectionCell removeFromConnectionQueue();
void *loggerThreadFunc(void *args);
void addToLogQueue(char *response);
char *removeFromLogQueue();
void addToConnectionBuffer(struct connectionCell new_connection);
int getPriority();
int isTheWordInDictionary(char *buffer);
void initializeConnectionQueue(int numberOfCells);
int checkIfInteger(char *str);
struct connectionCell getClientWithHighestPriority();
void runServer(int numberOfWorkerThreads, char *schedulingType);
char *removeSpace(char *str);
int validSchedulingType(char *sType);

// Here are my two locks, and 4 condition variables.
pthread_mutex_t connection_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t connection_queue_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t connection_queue_empty = PTHREAD_COND_INITIALIZER;

pthread_mutex_t log_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t log_queue_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t log_queue_empty = PTHREAD_COND_INITIALIZER;




struct connectionCell *connection_queue = NULL;
char *log_queue[QUEUE_SIZE];


int connection_queue_size = 0;
int log_queue_size = 0;

char *dictionaryFile;
int portNumber = 0;
int numberOfWorkerThreads = 0;
int numberOfCellsInConnectionQueue = 0;
char *schedulingType;
char **dictionaryWords = NULL;


/*
The main function is the server. It initializes the worker threads, and the logger threads.
Next it is checking how many parameters there are in the code. Based on the paramters it is assining value. 
If all 5 paramters are given then we have the file/path, portNumber, number of worker threads, 
number of cells in connection queue and then finally the scheduling type.
I am loading the dictionary file.
I am initializing the connection_queue, the log_queue by deault you said assume has the value 10.
Then, I am accepting clients to join and when they have joined, I am putting the new_connection 
which is a struct of type connectionCell into the connection_queue. 
This is where I am also assigning it a priority
*/ 

int main(int argc, char **argv) {
    pthread_t workerThread[numberOfWorkerThreads];
    pthread_t loggerThread;

    

    if (argc <= 1 || argc > 6) {
        printf("%s", "Incorrect Number of arguments provided");
        exit(EXIT_FAILURE);
    } else if (argc == 4) {
        //dictionaryFile = "/usr/share/dict/words";
        dictionaryFile = DEAULT_DICTIONARY;
        portNumber = DEFAULT_PORT_NUMBER;
        numberOfWorkerThreads = atoi(argv[1]);
        numberOfCellsInConnectionQueue = atoi(argv[2]);
        schedulingType = argv[3];
    } else {
        if (argc == 5) {
            if (checkIfInteger(argv[1])) {
                dictionaryFile = DEAULT_DICTIONARY;
                portNumber = atoi(argv[1]);
                numberOfWorkerThreads = atoi(argv[2]);
                numberOfCellsInConnectionQueue = atoi(argv[3]);
                if (!(validSchedulingType(argv[4]))) {
                    perror("Please enter a valid scheduling type FIFO/priority");
                    exit(EXIT_FAILURE);
                }
                schedulingType = argv[4];
            } else {
                dictionaryFile = argv[1];
                portNumber = DEFAULT_PORT_NUMBER;
                numberOfWorkerThreads = atoi(argv[2]);
                numberOfCellsInConnectionQueue = atoi(argv[3]);
                if (!(validSchedulingType(argv[4]))) {
                    perror("Please enter a valid scheduling type FIFO/priority");
                    exit(EXIT_FAILURE);
                }
                schedulingType = argv[4];
            }
        } else {
            dictionaryFile = argv[1];
            portNumber = atoi(argv[2]);
            numberOfWorkerThreads = atoi(argv[3]);
            numberOfCellsInConnectionQueue = atoi(argv[4]);
            if (!(validSchedulingType(argv[5]))) {
                perror("Please enter a valid scheduling type FIFO/priority");
                exit(EXIT_FAILURE);
            }
            schedulingType = argv[5];
        }
    }
    for (int i = 0; i < numberOfWorkerThreads; i++) {
        pthread_create(&workerThread[i], NULL, workerThreadFunc, (void*)schedulingType);
    }

    initializeConnectionQueue(numberOfCellsInConnectionQueue);
    pthread_create(&loggerThread, NULL, loggerThreadFunc, NULL);

  
    dictionaryWords = loadDictionary(dictionaryFile);

    int socket_desc, client_socket;
    struct sockaddr_in server, client;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0) {
        puts("Error creating socket");
        exit(1);
    }

    // declaring server of type sockaddr_in
    // with specific address family, IP Address and port number
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(portNumber);

    // binding a socket with an specific address and port. 
    int bind_result = bind(socket_desc, (struct sockaddr*)&server, sizeof(server));
    if (bind_result < 0) {
        puts("Error: failed to bind");
        exit(1);
    }
    puts("Bind done");

    int listenResult = listen(socket_desc, 3);
    puts("waiting for incoming connections...");
    if (listenResult < 0) {
        perror("Error listening on server socket");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int client_length = sizeof(client);
        client_socket = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&client_length);
        if (client_socket < 0) {
            puts("Error: Accept failed");
            continue;
        }

        puts("connection accepted");

        int priority = getPriority();
        struct connectionCell new_connection;
        new_connection.socketDescriptor = client_socket;
        new_connection.priority = priority;
        struct timespec timeClientJoined;
        clock_gettime(CLOCK_REALTIME, &timeClientJoined);
        new_connection.connectionTime = timeClientJoined.tv_sec * 1000 + timeClientJoined.tv_nsec/1000000;
       
        addToConnectionBuffer(new_connection);
        pthread_cond_signal(&connection_queue_full);
    }
    return 0;    
}


/*
This function initializes the connection Queue. 
Based on the parameter passed for the number of cells. 
The function dynamically allocates memory for an array of struct connectionCell.


*/
void initializeConnectionQueue(int numberOfCells) {
    connection_queue = (struct connectionCell*)malloc(numberOfCells *sizeof(struct connectionCell));
    //connection_queue_size = numberOfCells;
    //printf("connection_queue_size = %d, numberOfCells = %d\n", connection_queue_size, numberOfCells);
}

/*
The worker thread function receives the paramter of the scheduling type FIFO/priority
If the connection_queue is empty it will wait until there is a client.
If the scheduling type is FIFO:
    It will remove the first value from the top of the connection_queue since it is FIFO
    Next we will read from the client. 
    before sending message to dictionary we will clean the message removing any extra space.
    If he send message we check if it is in the dictionary.
    Based on the response, we will send a response to the client.
    while we are receving data from the client we will keep checking, and adding to log_queue.
    Once the client is done sending data we close client_desc using close.

If the scheduling type is priority:
    I get the client with the highest priority. 
    Next we will read from the client. 
    before sending message to dictionary we will clean the message removing any extra space.
    If he send message we check if it is in the dictionary.
    Based on the response, we will send a response to the client.
    while we are receving data from the client we will keep checking, and adding to log_queue.
    Once the client is done sending data we close client_desc using close.


*/

void *workerThreadFunc(void *args) {
    char *schedulingType = (char *)args;
    char buffer[250];
    //struct logCell logEntry;
    while (1) {
        pthread_mutex_lock(&connection_queue_mutex);
        while (connection_queue_size == 0) {
            pthread_cond_wait(&connection_queue_full, &connection_queue_mutex);
        }
        pthread_mutex_unlock(&connection_queue_mutex);
        printf("Lock gets released\n");
        //int temp = connection_queue_size;
        if (strcmp(schedulingType, "FIFO") == 0) {
            //printf("Entering FIFO\n");
            // for(int i = 0; i < temp; i++) {
            struct connectionCell connection_info = removeFromConnectionQueue();
            printf("priority = %d, socket = %d\n", connection_info.priority, connection_info.socketDescriptor);
            ssize_t bytes_received;
            while ((bytes_received = read(connection_info.socketDescriptor, buffer, sizeof(buffer))) > 0) {
                buffer[bytes_received] = '\0';  // Null-terminate the received data
                printf("Received: %s\n", buffer);
                int flag;
                char newBuffer[200];                
                char response[400];
                char *removeSpaceBuffer = removeSpace(buffer);
                strcpy(newBuffer, removeSpaceBuffer);
                free(removeSpaceBuffer);
                flag = isTheWordInDictionary(newBuffer);
                if (flag) {
                    snprintf(response, sizeof(response), "word = %s + status = OK + clientConnectionTime = %ld\n", newBuffer, connection_info.connectionTime);
                    printf("response = %s\n", response);
                } else {
                    printf("Word is not in the dictionary\n");
                    snprintf(response, sizeof(response), "word = %s + status = MISSPELLED + clientConnectionTime = %ld\n", newBuffer, connection_info.connectionTime);
                    printf("response = %s\n", response);
                }
                write(connection_info.socketDescriptor, response, strlen(response));
                addToLogQueue(response);
                pthread_cond_signal(&log_queue_full);
            }
            close(connection_info.socketDescriptor);
        }
        if (strcmp(schedulingType, "priority") == 0) {
            //printf("Entering priority\n");
            struct connectionCell connection_info = getClientWithHighestPriority();
            //printf("priority = %d, socket = %d\n", connection_info.priority, connection_info.socketDescriptor);
            ssize_t bytes_received;
            while ((bytes_received = read(connection_info.socketDescriptor, buffer, sizeof(buffer))) > 0) {
                buffer[bytes_received] = '\0';  // Null-terminate the received data
                printf("Received: %s\n", buffer);
                int flag;
                char newBuffer[250];                
                char response[2000];
                char *removeSpaceBuffer = removeSpace(buffer);
                strcpy(newBuffer, removeSpaceBuffer);
                free(removeSpaceBuffer);
                flag = isTheWordInDictionary(newBuffer);
                if (flag) {
                    snprintf(response, sizeof(response), "word = %s + status = OK + clientConnectionTime = %ld\n", newBuffer, connection_info.connectionTime);
                    printf("response = %s\n", response);
                } else {
                    printf("Word is not in the dictionary\n");
                    snprintf(response, sizeof(response), "word = %s + status = MISSPELLED + clientConnectionTime = %ld\n", newBuffer, connection_info.connectionTime);
                }
                write(connection_info.socketDescriptor, response, strlen(response));
                addToLogQueue(response);
                pthread_cond_signal(&log_queue_full);
            }
            close(connection_info.socketDescriptor);
        }
    }
    return NULL;
}



/*
This function takes in a file and load the words in a file and saves it in dictionaryWords.
It opens the dicFile, and then we are dynamically allocating memory for the array
and then dynamically allocating memory for each sting.
Next I am reading each line using the fgets function and then adding it to toReturn
Finally closing the file

*/

char **loadDictionary(char *dictionaryFile) {
   
    char **toReturn = NULL;
    FILE *dicFile = fopen(dictionaryFile, "r");
    if (dicFile == NULL) {
        perror("Error opening the file");
    }
    toReturn = (char **)malloc(MAX_NUMBER_OF_WORDS * sizeof(char*));
    if (toReturn == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < MAX_NUMBER_OF_WORDS; i++) {
        toReturn[i] = (char *)malloc(MAX_NUMBER_OF_WORDS * sizeof(char));
        if (toReturn[i] == NULL) {
            perror("Memory allocation failed PART2");
            exit(EXIT_FAILURE);
        }
    }
    int index = 0;
    char line[MAX_NUMBER_OF_WORDS];
    while (fgets(line, sizeof(line), dicFile) != NULL) {
        size_t length = strlen(line);
        if (line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }
        strcpy(toReturn[index], line);
        index++;
    }
    fclose(dicFile);
    return toReturn;
}

/*
This function adds new connections to the connection queue buffer.
if it's at max size it will wait for the queue to get empty. 


*/

void addToConnectionBuffer(struct connectionCell new_connection) {
    pthread_mutex_lock(&connection_queue_mutex);

    while(connection_queue_size == numberOfCellsInConnectionQueue) {
        pthread_cond_wait(&connection_queue_empty, &connection_queue_mutex);
    }
    connection_queue[connection_queue_size] = new_connection;
    connection_queue_size = connection_queue_size + 1;
    pthread_cond_signal(&connection_queue_full);
    pthread_mutex_unlock(&connection_queue_mutex);
    for(int i = 0; i < connection_queue_size; i++) {
        printf("i = %d, priority = %d, sd = %d\n", i, connection_queue[i].priority, connection_queue[i].socketDescriptor);
    }
}

/*
This function is for the FIFO scheduling type.
Getting the top value and then shifitng each elements down one.

*/


// Function to remove connection from the connection queue
struct connectionCell removeFromConnectionQueue() {
    struct connectionCell connect_info;
   
    connect_info = connection_queue[0];
    for(int i = 0; i < connection_queue_size - 1; i++) {
        connection_queue[i] = connection_queue[i+1];
    }
    connection_queue_size = connection_queue_size - 1;
    //pthread_mutex_unlock(&connection_queue_mutex);

    return connect_info;
}

/*
The logger Thread function gets the top value from the log queue and then puts in the queue.
If the log queue size is 0 the thread will sleep until something goes inside.
Next, I am opening the file, and appending values to teh log. 
I am using the fflush function which forces each value inside the file
finally closing the file

*/


void *loggerThreadFunc(void *args) {
    FILE *file;
    while (1) {
        char *response = NULL;
        pthread_mutex_lock(&log_queue_mutex);
        while(log_queue_size == 0) {
            pthread_cond_wait(&log_queue_full, &log_queue_mutex);
        }
        response = removeFromLogQueue();
        pthread_cond_signal(&log_queue_empty);
        pthread_mutex_unlock(&log_queue_mutex);
        printf("Just before file, value = %s\n", response);
        file = fopen("log.txt", "a");
        if (file == NULL) {
            perror("Error opening the logEntry file");
            exit(EXIT_FAILURE);
        }
        fprintf(file, "%s\n", response);
        fflush(file);
    }
    fclose(file);
    return NULL;
}

/*

This function add the response from the client inside the log queue.
If the log queue is full we will wait.
*/

void addToLogQueue(char *response) {
    
    pthread_mutex_lock(&log_queue_mutex);

    while (log_queue_size == QUEUE_SIZE) {
        pthread_cond_wait(&log_queue_empty, &log_queue_mutex);
    }
    log_queue[log_queue_size] = response;
    log_queue_size = log_queue_size + 1;
    pthread_cond_signal(&log_queue_full);
    pthread_mutex_unlock(&log_queue_mutex);
} 

/*
The remove from log queue, just returns the top value from the queue.
The elements shift down one value

*/

char *removeFromLogQueue() {
    char *toReturn;
    //pthread_mutex_lock(&log_queue_mutex);
    toReturn = log_queue[0];
    for (int i = 0; i < log_queue_size - 1; i++) {
        log_queue[i] = log_queue[i+1];
    }
    log_queue_size = log_queue_size - 1;
    

    return toReturn;
}

/*
Returns a random number which gives the priority
*/

int getPriority() {
    int randomNumber = 1 + rand() %(10 - 1 + 1);
    return randomNumber;
}

/*
Function checks if the word is in the dictionary.
If it is it will return 1, else 0
*/

int isTheWordInDictionary(char *buffer) {
    //printf("length of param = %lu\n", strlen(buffer));
    for (int i = 0; i < MAX_NUMBER_OF_WORDS; i++) {
        if (strcmp(buffer, dictionaryWords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
This function gets the client with the highest priority. 
It loops through all the conenction in the queues and gets the index with highest priority.
next we save that in the connect_info, and then remove it by shifting each element from that index
down one.


*/
struct connectionCell getClientWithHighestPriority() {
    printf("%s", "inside highest priority\n");
    int maxPriority = 0; 
    int maxPriorityIndex = -1;
    struct connectionCell connect_info;

    pthread_mutex_lock(&connection_queue_mutex);
    for(int i = 0; i < connection_queue_size; i++) {
        if (connection_queue[i].priority > maxPriority) {
            maxPriority = connection_queue[i].priority;
            maxPriorityIndex = i;
        }
    }
    if (maxPriorityIndex != -1) {
        connect_info = connection_queue[maxPriorityIndex];
        for (int i = maxPriorityIndex; i < connection_queue_size - 1; i++) {
            connection_queue[i] = connection_queue[i+1];
        }
        connection_queue_size = connection_queue_size - 1;
    } else {
        connect_info.socketDescriptor = -1;
        connect_info.priority = -1;

    }
    pthread_mutex_unlock(&connection_queue_mutex);
    return connect_info;
}

/*
This function checks if the string is an integer.
This is for one of the paramters I take above.
*/
int checkIfInteger(char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    char *endptr;
    strtol(str, &endptr, 10);
    // If strtol successfully parses the entire string, and the first character is not the null terminator,
    // then the string represents a valid integer
    if (*endptr == '\0') {
        return 1;
    } else {
        return 0;
    }

}

/*
This function just cleans the string and removes all the white spaces.
*/

char *removeSpace(char *str) {
    int lengthOfString = strlen(str);
    int j = 0;
    char *toReturn = (char *)malloc(lengthOfString + 1);
    
    for(int i = 0; i < lengthOfString; i++) {
        if (!isspace((unsigned char)str[i])) {
            toReturn[j] = str[i];
            j++;
        }
    }
    toReturn[j] = '\0';
    return toReturn;
}
/*
This function checks to makes sure you have a valid scheduling type.

*/
int validSchedulingType(char *sType) {
    if (strcmp(sType, "priority") == 0 || strcmp(sType, "FIFO") == 0) {
        return 1;
    } else {
        return 0;
    }
}




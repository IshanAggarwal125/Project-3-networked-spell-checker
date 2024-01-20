#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <time.h>

//#define PORT 2506
#define MESSAGE_SIZE 2000
int port = 0;

int main(int argc, char **argv) {
	port = atoi(argv[1]);
    int socket_desc;
    struct sockaddr_in server;
    char message[MESSAGE_SIZE];
    char replyFromServer[MESSAGE_SIZE];

    // Creating a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
        return 1;
    }

    // declaring server of type sockaddr_in
    // with specific address family, IP Address and port number
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;;
    server.sin_port = htons(port);

    // connecting to the server, using the socket
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        puts("connect error\n");
        return 1;
    }

    puts("Connected\n");

    while (1) {
        // Get user input for the message from the cmdline
        printf("Enter a word to check, type 'exit' to exit: ");
        fgets(message, MESSAGE_SIZE, stdin);
        // Remove the newline character from the input
        message[strcspn(message, "\n")] = '\0';

        // Check if the user wants to exit
        if (strcmp(message, "exit") == 0) {
            break;
        }
        if (strlen(message) == 0) {
            perror("The length of the message must be greater than 0, please reconnect!");
            break;
        }

        // sending the data which is the message to the socket. 
        if (send(socket_desc, message, strlen(message), 0) < 0) {
            puts("Send failed");
            return 1;
        }
        puts("Data Sent\n");

        // Receive a reply from the server
        ssize_t bytesReceivedFromServer = recv(socket_desc, replyFromServer, MESSAGE_SIZE, 0);
        if (bytesReceivedFromServer > 0) {
        // Null-terminate the received data
            replyFromServer[bytesReceivedFromServer] = '\0';

            // Process the received data
            printf("Reply from the server: %s\n", replyFromServer);
        } else if (bytesReceivedFromServer == 0) {
            // Connection closed by the server
            printf("Connection closed by the server\n");
            // Handle accordingly (e.g., break out of a loop or exit the program)
        } else {
            // Error in recv
            perror("recv");
        }
    }
    // Close the socket
    close(socket_desc);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main() {
   
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket binding failed");
        exit(1);
    }
    printf("Saransh\n");
    // Listen for incoming connections
    if (listen(server_socket, 10) == -1) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server listening on port 8080...\n");

    socklen_t client_addr_len = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

    if (client_socket == -1) {
        perror("Connection failed");
        exit(1);
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        recv(client_socket, buffer, sizeof(buffer), 0);
        printf("Received data: %s\n", buffer);

        // Your packet handling logic can go here

        if (strcmp(buffer, "exit") == 0) {
            printf("Connection closed by client.\n");
            break;
        }
    }

    close(server_socket);
    return 0;
}

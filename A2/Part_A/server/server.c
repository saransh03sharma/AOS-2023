#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

void handle_packet(char *buffer, int n) {
    // Print integer
    int *payloadData = (int *)buffer;
    printf("Received data: %d\n", *payloadData);
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create a socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
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

    printf("Server listening on port 8080...\n");

    while (1) {
        char buffer[1024];
        memset(buffer, 0, 1024);
        int n = recvfrom(server_socket, buffer, 1024, 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (n == -1) {
            perror("Receive failed");
            break;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            handle_packet(buffer, n);
            exit(0);
        }
    }

    close(server_socket);
    return 0;
}
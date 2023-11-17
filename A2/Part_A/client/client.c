#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main() {
    
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // Create a socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int i=0;
    int n;
    while(1) {
        // sprintf(buffer, "%d", i);
        memcpy(buffer, &i, sizeof(int));
        n = sendto(client_socket, buffer, sizeof(int), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (n != sizeof(int)) {
            perror("Send failed");
            break;
        }
        printf("Sent data: %d\n", i);
        // printf("Sent bytes: %d\n", n);
        i++;
        sleep(10); // Add a delay between consecutive sends
    }

    close(client_socket);
    return 0;
}
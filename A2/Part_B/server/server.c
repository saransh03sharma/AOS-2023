#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

#define NUM_THREADS 5
#define LOAD_BALANCER_PORT 11000

struct sockaddr_in balancer_addr;
struct sockaddr_in server_addr;
int server_socket;

void handler_thread(void * param) {
    int id = (uintptr_t)param;
    printf("Thread %d spawned\n", id);
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[30];
    int n;
    int total_sleep_time = 0;
    while(1){
        n = recvfrom(server_socket, buffer, 30, 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (n == -1) {
            printf("Receive failed at thread %d\n", id);
            break;
        }
        int *payloadPointer = (int *)buffer;
        int payload = *payloadPointer;
        if(payload <= 0) {
            printf("Thread %d received invalid payload %d\n", id, payload);
            break;
        }
        printf("Thread %d sleeping for %d seconds\n", id, payload);
        total_sleep_time += payload;
        sleep(payload);
        printf("Thread %d awake, total sleep time of this thread: %d\n", id, total_sleep_time);
        *payloadPointer = 0;
        n = sendto(server_socket, buffer, sizeof(int), 0, (struct sockaddr*)&balancer_addr, sizeof(balancer_addr));
        if (n!=sizeof(int)) {
            perror("Send failed");
            break;
        }
        printf("Thread %d is free and has notified the load balancer\n", id);
    }
    printf("Thread %d exiting\n", id);
    pthread_exit(NULL);
    
}

int main() {
    // int server_socket;
    // struct sockaddr_in server_addr, client_addr;
    // socklen_t client_addr_len = sizeof(client_addr);

    // Create a socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // Input the port number to bind
    int port;
    printf("Enter port number to bind: ");
    scanf("%d", &port);
    if(port < 1024 || port > 65535) {
        printf("Invalid port number\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket binding failed");
        exit(1);
    }

    printf("Server listening on port %d...\n", port);

    // Connect to the load balancer
    balancer_addr.sin_family = AF_INET;
    balancer_addr.sin_port = htons(LOAD_BALANCER_PORT);
    balancer_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Spawn threads
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, handler_thread, (void *)(uintptr_t)i);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
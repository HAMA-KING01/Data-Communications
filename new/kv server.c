#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8081
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024

typedef struct {
    char key[50];
    char value[50];
} KeyValue;

KeyValue store[100];
int store_count = 0;

void *handle_client(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int n;

    while ((n = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        buffer[n] = '\0'; // Null-terminate the received string
        char command[10], key[50], value[50];

        if (sscanf(buffer, "%s %s %s", command, key, value) == 3 && strcmp(command, "SET") == 0) {
            strcpy(store[store_count].key, key);
            strcpy(store[store_count].value, value);
            store_count++;
            send(sock, "Stored", strlen("Stored"), 0);
        } else if (sscanf(buffer, "%s %s", command, key) == 2 && strcmp(command, "GET") == 0) {
            int found = 0;
            for (int i = 0; i < store_count; i++) {
                if (strcmp(store[i].key, key) == 0) {
                    send(sock, store[i].value, strlen(store[i].value), 0);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                send(sock, "Key not found", strlen("Key not found"), 0);
            }
        } else {
            send(sock, "Invalid command", strlen("Invalid command"), 0);
        }
    }

    close(sock);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind the socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected\n");
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, (void *)&new_socket);
    }

    return 0;
}
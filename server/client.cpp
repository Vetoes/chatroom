#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080

void *receive_messages(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[1024];
    int bytes_read;

    while ((bytes_read = read(client_socket, buffer, 1024)) > 0) {
        buffer[bytes_read] = '\0';
        std::cout << "Server: " << buffer << std::endl;
    }

    return nullptr;
}

int main() {
    int client_socket;
    struct sockaddr_in server_address;

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        close(client_socket);
        return -1;
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(client_socket);
        return -1;
    }

    // Create a thread to receive messages from server
    pthread_t receive_thread;
    pthread_create(&receive_thread, nullptr, receive_messages, (void *)&client_socket);
    pthread_detach(receive_thread);

    // Send messages to server
    char message[1024];
    while (true) {
        std::cin.getline(message, 1024);
        send(client_socket, message, strlen(message), 0);
    }

    close(client_socket);
    return 0;
}

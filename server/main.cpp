#include "server.h"
/*#include "server.cpp"*/
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

#define PORT 8080
#define BUFFER_SIZE 1024

void receive_messages(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_read] = '\0';
        std::cout << buffer << std::endl;
    }

    if (bytes_read == 0) {
        std::cout << "Server disconnected" << std::endl;
    } else {
        perror("recv failed");
    }

    close(client_socket);
    exit(0);
}

int main() {
    int client_socket;
    struct sockaddr_in server_address;

    // 소켓 생성
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return 1;
    }

    // 서버에 연결
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection to server failed");
        return 1;
    }

    std::cout << "Connected to the server.\n";

    // 메시지 수신 스레드 시작
    std::thread recv_thread(receive_messages, client_socket);
    recv_thread.detach();

    // 사용자 입력 처리
    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "/exit") {
            break;
        }
        send(client_socket, message.c_str(), message.length(), 0);
    }

    // 연결 종료
    close(client_socket);
    return 0;
}


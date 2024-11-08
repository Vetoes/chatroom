#include <iostream>
#include <fstream> // For logging messages
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <algorithm> // Include algorithm for std::remove
#include <ctime> // For timestamp

#define PORT 8080
#define MAX_CLIENTS 10

class User {
public:
    int socket;
    std::string username;

    User(int socket, const std::string& username = "") : socket(socket), username(username) {}
};

std::vector<User> users;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<std::string> banned_users;

std::string get_timestamp() {
    std::time_t now = std::time(nullptr);
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", std::localtime(&now));
    return std::string(buf);
}

std::ofstream server_log("server_log_" + get_timestamp() + ".txt", std::ios::app);

void log_message(const std::string &message) {
    if (server_log.is_open()) {
        server_log << message << std::endl;
    }
}

void broadcast_message(const std::string& message, int sender_socket) {
    pthread_mutex_lock(&users_mutex);
    for (const auto& user : users) {
        if (user.socket != sender_socket) {
            send(user.socket, message.c_str(), message.length(), 0);
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

void send_private_message(const std::string& target_username, const std::string& message, int sender_socket) {
    pthread_mutex_lock(&users_mutex);
    bool user_found = false;
    for (const auto& user : users) {
        if (user.username == target_username) {
            send(user.socket, message.c_str(), message.length(), 0);
            user_found = true;
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);

    if (!user_found) {
        std::string error_message = "User " + target_username + " not found.\n";
        send(sender_socket, error_message.c_str(), error_message.length(), 0);
    }
}

void banish_user(const std::string& target_username, int requester_socket){
  pthread_mutex_lock(&users_mutex);
  auto it = std::find_if(users.begin(),users.end(),[&target_username](const User& user){
    return user.username == target_username;
  });

  if( it != users.end()){
    //Disconnt the user and remove from the list
    close(it->socket);
    banned_users.push_back(it->username);
    users.erase(it);

    std::string ban_message = target_username + " has been banished from the chat\n";
    log_message(ban_message);
    broadcast_message(ban_message , requester_socket);
  } else {
    std::string error_message = "User " + target_username + " not found.\n";
    send(requester_socket,error_message.c_str(),error_message.length(),0);
  }
  pthread_mutex_unlock(&users_mutex);

}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
  std::cout << client_socket <<std::endl;
    char buffer[1024];
    int bytes_read;

    // Ask for username
    send(client_socket, "Enter your username: ", 21, 0);
    bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[bytes_read] = '\0';
    std::string username(buffer);
    username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());

    pthread_mutex_lock(&users_mutex);

  /*  if(std::find(banned_users.begin(),banned_users.end(),username) != banned_users.end()){*/
  /*  std::string ban_message = "You are banished from this server.\n";*/
  /*  send(client_socket, ban_message.c_str(),ban_message.length(),0);*/
  /*  close(client_socket);*/
  /*  pthread_mutex_unlock(&users_mutex);*/
  /*  return nullptr;*/
  /*}*/

    users.emplace_back(client_socket, username);
    pthread_mutex_unlock(&users_mutex);

    std::string join_message = username + " has joined the chat\n";
    log_message(join_message);
    broadcast_message(join_message, client_socket);

    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_read] = '\0';
        std::string message(buffer);

    std::cout << "Client " << username << " input: " << message << std::endl;

        std::string command;
        if (message.find("@") == 0) {
            command = "whisper";
        } else if (message.find("/b ") == 0) {
            command = "banish";
        } else {
            command = "broadcast";
        }

        switch (command[0]) {
            case 'w': { // Whisper command
                size_t colon_pos = message.find(' ');
                if (colon_pos != std::string::npos) {
                    std::string target_username = message.substr(1, colon_pos - 1);
                    std::string private_message = "[Whisper from " + username + "]: " + message.substr(colon_pos + 1);
                    send_private_message(target_username, private_message, client_socket);
                }
                break;
            }
            /*case 'b': { // Banish command*/
            /*    std::string target_username = message.substr(8);*/
            /*    banish_user(target_username, client_socket);*/
            /*    break;*/
            /*}*/
            default: { // Broadcast message
                std::string broadcast_msg = username + ": " + message;
                log_message(broadcast_msg);
                broadcast_message(broadcast_msg, client_socket);
                break;
            }
        }
    }

    // Client disconnected
    close(client_socket);
    pthread_mutex_lock(&users_mutex);
    users.erase(std::remove_if(users.begin(), users.end(), [client_socket](const User& user) {
        return user.socket == client_socket;
    }), users.end());
    pthread_mutex_unlock(&users_mutex);

    std::string leave_message = username + " has left the chat\n";
    log_message(leave_message);
    broadcast_message(leave_message, client_socket);

    return nullptr;
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, nullptr, handle_client, (void*)&client_socket);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}


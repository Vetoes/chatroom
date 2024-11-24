// server.cpp
#include "server.h"
#include <iostream>
#include <fstream> // For logging messages
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <sstream>
#include <algorithm> // Include algorithm for std::remove
#include <ctime> // For timestamp

std::vector<User> users;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void banish_user(const std::string& target_username, int requester_socket) {
    pthread_mutex_lock(&users_mutex);
    auto requester_it = std::find_if(users.begin(), users.end(), [requester_socket](const User& user) {
        return user.socket == requester_socket;
    });

    if (requester_it == users.end() || (requester_it->power != User::SUPER && requester_it->power != User::MANAGER)) {
        std::string error_message = "You do not have permission to banish users.\n";
        send(requester_socket, error_message.c_str(), error_message.length(), 0);
        pthread_mutex_unlock(&users_mutex);
        return;
    }
    auto it = std::find_if(users.begin(), users.end(), [&target_username](const User& user) {
        return user.username == target_username;
    });

    if (it != users.end()) {
        if (it->power == User::SUPER) {
            std::string error_message = "Cannot banish a SUPER user.\n";
            send(requester_socket, error_message.c_str(), error_message.length(), 0);
        } else {
            // Set the user as banned and disconnect them
            it->is_ban = true;
            close(it->socket);
            std::string ban_message = target_username + " has been banished from the chat\n";
            log_message(ban_message);
            broadcast_message(ban_message, requester_socket);
            users.erase(it);
        }
    } else {
        std::string error_message = "User " + target_username + " not found.\n";
        send(requester_socket, error_message.c_str(), error_message.length(), 0);
    }
    pthread_mutex_unlock(&users_mutex);
}
void print_user_status(const std::string& target_username,int requester_socket) {
    pthread_mutex_lock(&users_mutex);
    auto user_it = std::find_if(users.begin(), users.end(), [&target_username](const User& user) {
        return user.username == target_username;
    });

    if (user_it != users.end()) {
        std::string status_message = "Username: " + user_it->username + "\n";
        status_message += "Power Level: ";
        switch (user_it->power) {
            case User::SUPER:
                status_message += "SUPER\n";
                break;
            case User::MANAGER:
                status_message += "MANAGER\n";
                break;
            case User::NOMAL:
                status_message += "NORMAL\n";
                break;
        }
        status_message += "Subscription Level: ";
        switch (user_it->subscribe_level) {
            case User::TIER_3:
                status_message += "TIER 3\n";
                break;
            case User::TIER_2:
                status_message += "TIER 2\n";
                break;
            case User::TIER_1:
                status_message += "TIER 1\n";
                break;
            case User::TIER_0:
                status_message += "TIER 0\n";
                break;
        }
        send(requester_socket, status_message.c_str(), status_message.length(), 0);
    } else{
    std::string error_message = "User " + target_username + " not found.";
    send(requester_socket,error_message.c_str(),error_message.length(),0);
  }
    pthread_mutex_unlock(&users_mutex);
}

void set_manager(const std::string& target_username, int requester_socket) {
    pthread_mutex_lock(&users_mutex);
    auto requester_it = std::find_if(users.begin(), users.end(), [requester_socket](const User& user) {
        return user.socket == requester_socket; });

    if (requester_it == users.end() || requester_it->power != User::SUPER) {
        std::string error_message = "You do not have permission to set managers.\n";
        send(requester_socket, error_message.c_str(), error_message.length(), 0);
        pthread_mutex_unlock(&users_mutex);
        return;
    }

    auto target_it = std::find_if(users.begin(), users.end(), [&target_username](const User& user) {
        return user.username == target_username;
    });

    if (target_it != users.end()) {
        target_it->power = User::MANAGER;
        std::string success_message = target_username + " has been promoted to MANAGER.\n";
        send(requester_socket, success_message.c_str(), success_message.length(), 0);
        broadcast_message(success_message, requester_socket);
    } else {
        std::string error_message = "User " + target_username + " not found.\n";
        send(requester_socket, error_message.c_str(), error_message.length(), 0);
    }
    pthread_mutex_unlock(&users_mutex);
}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    delete (int*)arg;
    char buffer[1024];
    int bytes_read;

    // Ask for username
    send(client_socket, "Enter your username: ", 21, 0);
    bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) {
        close(client_socket);
        return nullptr;
    }
    buffer[bytes_read] = '\0';
    std::string username(buffer);
    username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());

    pthread_mutex_lock(&users_mutex);
    if (std::any_of(users.begin(), users.end(), [&username](const User& user) { return user.username == username && user.is_ban; })) {
        std::string ban_message = "You are banished from this server.\n";
        send(client_socket, ban_message.c_str(), ban_message.length(), 0);
        close(client_socket);
        pthread_mutex_unlock(&users_mutex);
        return nullptr;
    }
    users.emplace_back(client_socket, username, false, false, users.empty() ? User::SUPER : User::NOMAL);
    pthread_mutex_unlock(&users_mutex);

    std::string join_message = username + " has joined the chat\n";
    log_message(join_message);
    broadcast_message(join_message, client_socket);

    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_read] = '\0';
        std::string message(buffer);

        // Print the client input to the console
        std::cout << "Client " << username << " input: " << message << std::endl;

        std::istringstream message_stream(message);
        std::string command;
        message_stream >> command;

        if (command == "@") { // Whisper command detected
            std::string target_username;
            message_stream >> target_username;
            std::string private_message;
            std::getline(message_stream, private_message);
            private_message = "[Whisper from " + username + "]: " + private_message;
            send_private_message(target_username, private_message, client_socket);
        } else if (command == "/banish") { // Banish command detected
            std::string target_username;
            message_stream >> target_username;
            banish_user(target_username, client_socket);
        } else if (command == "/setmanager") { // Set manager command detected
            std::string target_username;
            message_stream >> target_username;
            set_manager(target_username, client_socket);
        } else if (command == "/status") { // Set manager command detected
            std::string target_username;
            message_stream >> target_username;
            print_user_status(target_username, client_socket);
        } else {
            std::string broadcast_msg = username + ": " + message;
            log_message(broadcast_msg);
            broadcast_message(broadcast_msg, client_socket);
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


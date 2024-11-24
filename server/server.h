// server.hpp
#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <pthread.h>

class User {

public:

    int socket;
    std::string username;
    bool is_ban;
    bool followed;


    enum Power {
        SUPER,
        MANAGER,
        NOMAL,
    }power;

    enum SubscribeLevel {
        TIER_0,
        TIER_1,
        TIER_2,
        TIER_3,
    }subscribe_level;

    User(int socket, const std::string& username = "", bool is_ban = false, bool followed = false, Power power = NOMAL, SubscribeLevel level = TIER_0)
        : socket(socket), username(username), is_ban(is_ban), followed(followed), power(power), subscribe_level(level) {}

  void broadcast_message(const std::string& message, int sender_socket);
};

extern std::vector<User> users;
extern pthread_mutex_t users_mutex;

std::string get_timestamp();
void log_message(const std::string& message);
void send_private_message(const std::string& target_username, const std::string& message, int sender_socket);
void print_user_status(const std::string& target_username,int requester_socket);
void banish_user(const std::string& target_username, int requester_socket);
void set_manager(const std::string& target_username, int requester_socket);
void* handle_client(void* arg);
#endif // SERVER_HPP

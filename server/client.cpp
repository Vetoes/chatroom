#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctime>

#define PORT 8080


std::string get_timesptamp(){
  std::time_t now = std::time(nullptr);
  char buf[80];
  std::strftime(buf,sizeof(buf), "%Y-%m-%d %H:%M:%S",std::localtime(&now));
  return std::string(buf);
}

void *receive_messages(void *arg) {
  int client_socket = *(int *)arg;
  char buffer[1024];
  int bytes_read;

  /*std::ofstream chat_log("chat_log"+get_timesptamp()+".txt",std::ios::app);*/
  /*if(!chat_log){*/
  /*  std::cerr << "failed to open chat log file" << std::endl;*/
  /*  return nullptr;*/
  /*}*/

  while ((bytes_read = read(client_socket, buffer, 1024)) > 0) {
    buffer[bytes_read] = '\0';
    std::string receive_message = "[server] " + std::string(buffer);
    std::cout << receive_message << std::endl;
    /*chat_log << receive_message << std::endl;*/
    /*std::cout << "\r" << receive_message << "\n> " <<std::flush;*/

  }
  /*chat_log.close();*/

  return nullptr;
}



int main() {
  int client_socket;
  struct sockaddr_in server_address;
  std::string username;

  /*std::cout << "Enter your username: ";*/
  /*std::getline(std::cin,username);*/

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

  /*std::ofstream chat_log("chat_log.txt",std::ios::app);*/
  /*if(!chat_log){*/
  /*  std::cerr << "Failed to open chat log file" << std::endl;*/
  /*  close(client_socket);*/
  /*  return -1;*/
  /*}*/
    // Send messages to server
    char message[1024];
    while (true) {
    /*std::cout << "> ";*/
    /*if(!std::cin.getline(message,1024)){*/
    /*  std::cout << "Exiting chat..." <<std::endl;*/
    /*  break;*/
    /*}*/
    if(std::string(message) == "/quit"){
      std::cout << "Exiting chat..." << std::endl;
      break;
    }

    std::string timestamp =get_timesptamp();
    std::string formatted_message = "[" + timestamp + "] " + username + ": " +message;

    /*chat_log << formatted_message <<std::endl;*/

        std::cin.getline(message, 1024);
        /*send(client_socket, message, strlen(message), 0);*/
        send(client_socket, formatted_message.c_str(), formatted_message.length(), 0);
    }
  /*chat_log.close();*/
    close(client_socket);
    return 0;
}

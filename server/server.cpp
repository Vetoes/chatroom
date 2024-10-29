#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <algorithm>
#include <ctime>

#define PORT 8080
#define MAX_CLIENTS 10

std::vector<int> clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


std::string get_timestamp(){
  std::time_t now = std::time(nullptr);
  char buf[80];
  //timeptr = localtime(&now);
  //std::strftime(buf, sizeof(buf),"%Y-%m-%d_%H-%M-%S",timeptr);
  std::strftime(buf, sizeof(buf),"%Y-%m-%d_%H-%M-%S",std::localtime(&now));
  return std::string(buf);
}

void *handle_client(void *arg){
  int client_socket = *(int *)arg;
  delete (int*)arg;
  char buffer[1024];
  int bytes_read;

  std::ofstream client_log("client_log"+ get_timestamp()+".txt",std::ios::app);

  if(!client_log){
    std::cerr << "Failed to open client log file" << std::endl;
    return nullptr;
  }

  while((bytes_read =  read(client_socket,buffer, 1024)) > 0) { 
    buffer[bytes_read] = '\0';
    std::string received_message = "Client: " + std::string(buffer);
    std::cout << received_message << std::endl;
    client_log << received_message << std::endl;


    pthread_mutex_lock(&clients_mutex);
    for(int client : clients){
      if(client != client_socket){
        send(client, buffer, bytes_read,0);
      }
    }
    pthread_mutex_unlock(&clients_mutex);
  }

  client_log.close();
    close(client_socket);
    clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
    pthread_mutex_unlock(&clients_mutex);
    return nullptr;
  }

int main(){
  int server_fd, new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  std::string log_filename = "server_log_"+get_timestamp()+".txt";
  std::ofstream server_log(log_filename,std::ios::app);

  if(!server_log){
    std::cerr << "Failed to open server log file" << std::endl;
    return -1;
  }

  if((server_fd = socket(AF_INET, SOCK_STREAM,0)) == 0) {
    std::cerr << "Socket creation failed" << std::endl;
    return -1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
    std::cerr << "Binding failed" << std::endl;
    close(server_fd);
    return -1;
  }

  if(listen(server_fd,MAX_CLIENTS) < 0){
    std::cerr << "Listen failed" << std::endl;
    close(server_fd);
    return -1;
  }

  std::cout << "Server is listening on port " << PORT << std::endl;
  server_log << "Server started at" << get_timestamp() <<std::endl;

  while(true){
    if(( new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0){
      std::cerr << "Accept failed" << std::endl;
      continue;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&address.sin_addr, client_ip,INET_ADDRSTRLEN);
    int client_port = ntohs(address.sin_port);

    std::cout << "New client connected: " << client_ip << ":" << client_port << std::endl;
    server_log << "New client connected at" << get_timestamp() << "- IP:" << client_ip << ", Port:"<< client_port <<std::endl;

    pthread_mutex_lock(&clients_mutex);
    clients.push_back(new_socket);
    pthread_mutex_unlock(&clients_mutex);

    pthread_t thread;

    int *new_sock = new int(new_socket);
    if(pthread_create(&thread, nullptr, handle_client, (void *)new_sock) !=0){
      std::cerr << "Failed to create thread" << std::endl;
      delete new_sock;
    }else{
      pthread_detach(thread);
    }
  }

  server_log.close();
  close(server_fd);
  return 0;
}

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[1024] = {0};
  //creating socket file descriptor
  if((server_fd =  socket(AF_INET, SOCK_STREAM,0)) = 0) {
    std::cerr << "Socket creation failed" << std::endl;
    return -1;
  }

  // forcefullty attaching socket to the port 8080
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "setsockopt failed" << std::endl;
        close(server_fd);
        return -1;
    }

  address.sin_family = AF_INET; //ipv4
  address.sin_addr.s_addr = INADDR_ANY;  //bind to any avilable network interface
  address.sin_port = htons(8080); //port number in network byte order
  

  if(bind(server_fd, (struct sockaddr *)&address,sizeof(address)) < 0){
    std::cerr << "Binding failed" << std::endl;
    close(server_fd);
    return -1;
  }
}

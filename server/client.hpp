#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <arpa/inet.h>

class Client{
public:
  Client(const std::string &server_ip,int server_port);
};


#endif

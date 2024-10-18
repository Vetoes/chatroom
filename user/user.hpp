#ifndef USER
#define USER

#include <string>
#include <ctime>

class User {
private:
  //todo std 
  std::string username; 
  std::string user_id;
  bool is_online;
  std::time_t last_active;

public:
  //
  User();
  User(const std::string& username, const std::string& user_id);

  ~User();

  // Getters
  std::string getUsername() const;
  std::string getUserId() const;
  bool getOnlineStatus() const;
  std::time_t getLastActive() const;

  // Setters
  void setUsername(const std::string& username);
  void setOnlineStatus(bool status);
  void updateLastActive();


  void sendMessage(const std::string& recipient_id,const std::string& message_content);
  void receiveMessage(const std::string& sender_id,const std::string& message_content);

  std::string toString() const;
};


#endif

#include <string>
#include <vector>

class User {
public:
  User() {};
  std::string firstName;
  std::string lastName;
  std::vector<std::string> identifiers;  
  int userID;
};

class Device {
public:
  Device() {};
  std::string uuid;
  std::string location;
};

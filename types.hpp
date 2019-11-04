#include <string>
#include <vector>

class User {
public:
  User() {};
  User(int userID, JsonObject obj);

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

class Registration {
public:
  Registration() {};
  Registration(int uid, int t, std::string dev) : userID(uid), timestamp(t), device(dev) {};

  FirebaseJson* ToJSON() const {
    FirebaseJson json;
    json.addInt("timestamp", timestamp);
    json.addInt("userID", userID);
    json.addString("device", device.c_str());
    return &json;
  }

  int userID;
  int timestamp;
  std::string device;
};
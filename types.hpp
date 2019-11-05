#include <string>
#include <vector>
#include <ArduinoJson.h>
#include <FirebaseESP32.h>

class User {
public:
  User() {};
  // TODO: Use FirebaseJSON consistently here for parsing the data, instead of this extra library
  User(int uid, const JsonObject& obj) {
      userID = uid;
      const char* str = obj["firstName"];
      firstName = std::string(str);
      str = obj["lastName"];
      lastName = std::string(str);
      JsonArray idArr = obj["identifiers"].as<JsonArray>();
      for (JsonVariant value : idArr) {
        if (value.isNull()) {
          continue;
        }
        str = value.as<char*>();
        identifiers.push_back(std::string(str));
      }
  };

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

  // TODO: Make this more ideomatic...
  FirebaseJson* ToJSON() {
    json = new FirebaseJson();
    json->addInt("timestamp", timestamp);
    json->addInt("userID", userID);
    json->addString("device", device.c_str());
    return json;
  }

  int userID;
  int timestamp;
  std::string device;
private:
  FirebaseJson* json;
};

#include <string>
#include <vector>
#include <sstream>
#include <ArduinoJson.h>
#include <FirebaseESP32.h>

std::string formatTimestamp(int timestamp) {
  time_t secondsSinceEpoch = timestamp;
  char str[10];
  strftime(str, sizeof str, "%Y%m%d", gmtime(&secondsSinceEpoch));
  return std::string(str);
}

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

  bool PushToFirebase(FirebaseData* firebaseData) {
    // Note: ignoring device for now
    std::stringstream ss;
    ss << "/registrations/" << userID << "-" << formatTimestamp(timestamp) << "/timestamp";
    Serial.print("Pushing JSON to path: "); Serial.println(ss.str().c_str());

    if (Firebase.setInt(*firebaseData, ss.str().c_str(), timestamp)) {
      Serial.println(firebaseData->dataPath());
      Serial.println(firebaseData->pushName());
      return true;
    } else {
      Serial.println(firebaseData->errorReason());
      return false;
    }
  }

  int userID;
  int timestamp;
  std::string device;
};

class NewCard {
public:
  NewCard() {};
  NewCard(int ts, const std::string& cardID) : timestamp(ts), identifier(cardID) {}

  bool PushToFirebase(FirebaseData* firebaseData) {
    auto json = new FirebaseJson();
    json->addInt("timestamp", timestamp);
    json->addString("identifier", identifier.c_str());
    if (Firebase.pushJSON(*firebaseData, "/newCards", *json)) {
      Serial.println(firebaseData->dataPath());
      Serial.println(firebaseData->pushName());
      return true;
    } else {
      Serial.println(firebaseData->errorReason());
      return false;
    }
  }

  int timestamp;
  std::string identifier;
};

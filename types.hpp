#include <vector>
#include <sstream>
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
  // Old ArduinoJson implementation
  /*User(int uid, const JsonObject& obj) {
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
  };*/
  User(int uid, String fName, String lName, std::vector<String> ids) {
    userID = uid;
    firstName = fName;
    lastName = lName;
    identifiers = ids;
  }

  String firstName;
  String lastName;
  std::vector<String> identifiers;  
  int userID;
};

class Registration {
public:
  Registration() {};
  Registration(int uid, int t) : userID(uid), timestamp(t) {};

  bool PushToFirebase(FirebaseData* firebaseData) {
    // Note: ignoring device for now
    std::stringstream ss;
    ss << "/registrations/" << userID << "-" << formatTimestamp(timestamp) << "/timestamp";
    Serial.print("Pushing JSON to path: "); Serial.println(ss.str().c_str());
    Serial.println(timestamp);

    if (Firebase.setInt(*firebaseData, ss.str().c_str(), timestamp)) {
      Serial.println("waiting done, success");
      Serial.println(firebaseData->dataPath());
      Serial.println(firebaseData->pushName());
      return true;
    } else {
      Serial.println("waiting done, fail");
      Serial.println(firebaseData->errorReason());
      return false;
    }
  }

  int userID;
  int timestamp;
};

class NewCard {
public:
  NewCard() {};
  NewCard(int ts, String cardID) : timestamp(ts), identifier(cardID) {}
  bool PushToFirebase(FirebaseData* firebaseData) {
    FirebaseJson json;
    json.set("timestamp", timestamp);
    json.set("identifier", identifier.c_str());

    Serial.print("Pushing JSON to path /newCards: "); Serial.print(timestamp); Serial.println(identifier.c_str());
    if (Firebase.pushJSON(*firebaseData, "/newCards", json)) {
      Serial.println("waiting done, success");
      Serial.println(firebaseData->dataPath());
      Serial.println(firebaseData->pushName());
      return true;
    } else {
      Serial.println("waiting done, failed");
      Serial.println(firebaseData->errorReason());
      return false;
    }
  }

  int timestamp;
  String identifier;
};

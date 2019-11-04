#include "types.hpp"
#include <ArduinoJson.h>

// Construct an user based off a JSON object
User::User(int uid, JsonObject obj) {
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
}
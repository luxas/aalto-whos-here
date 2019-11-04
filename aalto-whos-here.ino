/*
   Good links and resources:
   https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
   https://blog.atlasrfidstore.com/near-field-communication-infographic
   https://randomnerdtutorials.com/security-access-using-mfrc522-rfid-reader-with-arduino/
   https://www.instructables.com/id/ESP32-With-RFID-Access-Control/
   SPI pins on ESP32: https://github.com/espressif/arduino-esp32/blob/master/variants/esp32/pins_arduino.h#L20-L23
   https://seeeddoc.github.io/NFC_Shield_V1.0/
   https://github.com/elechouse/PN532/tree/PN532_HSU
   http://www.elechouse.com/elechouse/index.php?main_page=product_info&cPath=90_93&products_id=2242
   https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/
   https://arduinojson.org/v6/api/jsonvariant/
*/

// Install this by adding https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json to File -> Preferences -> Additional Board Manager URLs
// Then go to the board manager and add ESP32 support. We used version 1.0.4
#include <Arduino.h>
// Connect to Firebase through WiFi
#include <WiFi.h>
// Install this as Firebase ESP32 Client by Mobitz, version 3.2.1
// This also depends on curl -sSL https://github.com/mobizt/HTTPClientESP32Ex/archive/1.1.3.tar.gz | tar -xz -C ~/Arduino/libraries/
#include <FirebaseESP32.h>

// Connect to the NFC reader through SPI
#include <SPI.h>
// Installed 10.10.2019 at commit f9c62da0811d154f55e2169db4761a093c603726 using the following command:
// curl -sSL https://github.com/Seeed-Studio/PN532/archive/arduino.tar.gz | tar -xz PN532-arduino/PN532 PN532-arduino/PN532_SPI --strip-components=1 -C ~/Arduino/libraries/
// This was also a bit annoying, as we had to do a sed "s|PN532/PN532/||g" across all files downloaded
#include <PN532_SPI.h>
#include <PN532.h>
// Include secret constants
#include "secrets.h"
// Include our types
#include "types.hpp"

#include <iomanip>
#include <string>
#include <sstream>
#include <ArduinoJson.h>
#include <map>

#include <NTPClient.h>
#include <WiFiUdp.h>

const std::string UUID ="foobar";
// only allow one card registration per hour (currently hardcoded to a minute for testing)
const unsigned long registrationPeriod = 1 * 60; //  * 60;

// Setup variables for communicating with the NFC device
int nfcSPIPort = SS;
PN532_SPI pn532spi(SPI, nfcSPIPort);
PN532 nfc(pn532spi);

// Setup a holder variable for accessing Firebase
FirebaseData firebaseData;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

std::vector<User> users;
Device currentDevice;

std::map<std::string, unsigned long> registrations;

void setup() {
  // Open a serial connection to a computer, if connected
  Serial.begin(115200);

  // Open the SPI connection to the NFC device, using the default SS port (5 on the ESP32)
  setupNFC(nfc);

  // Connect to WiFi using the given SSID and password
  connectToWiFi(WIFI_SSID, WIFI_PASSWORD);

  // With internet connectivity set up, we can now connect to the Firebase Realtime Database
  initFirebase(firebaseData, FIREBASE_HOST, FIREBASE_SECRET);

  timeClient.begin();

  streamFirebaseData(firebaseData, "/users", usersCallback);
  /*std::string deviceStr = "/devices/";
  deviceStr += UUID;
  streamFirebaseData(firebaseData, deviceStr.c_str(), deviceCallback);*/
}

void setupNFC(PN532& nfcDevice) {
  Serial.println("Looking for a PN532 device on the SPI interface on port " + nfcSPIPort);

  nfcDevice.begin();

  uint32_t versiondata = nfcDevice.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board yet, waiting");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfcDevice.SAMConfig();

  Serial.println("Found and configured NFC device, waiting for an ISO14443A Card ...");
}

void connectToWiFi(char ssid[], char password[]) {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void initFirebase(FirebaseData& db, String host, String secret) {
  // put your setup code here, to run once:
  //4. Setup Firebase credential in setup()
  Firebase.begin(host, secret);

  //5. Optional, set AP reconnection in setup()
  Firebase.reconnectWiFi(true);

  //6. Optional, set number of auto retry when read/store data
  Firebase.setMaxRetry(db, 3);

  //7. Optional, set number of read/store data error can be added to queue collection
  Firebase.setMaxErrorQueue(db, 30);
}

void loop() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (!success) {
    Serial.print(timeClient.getFormattedTime()); Serial.println(": no card at the moment");
    sleep(2);
    return;
  }
  // Display some basic information about the card
  Serial.println("Found an ISO14443A card");
  Serial.print("  Card ID: ");
  std::string cardID = "";
  for (uint8_t i = 0; i < uidLength; i++) {
    if (i > 0) {
      cardID += ":";
    }
    char buf[2];
    sprintf(buf, "%02hhx", uid[i]);
    cardID += buf;
  }
  Serial.println(cardID.c_str());
  auto it = registrations.find(cardID);
  if (it != registrations.end()) {
    auto t = registrations[cardID];
    auto delta = timeClient.getEpochTime() - t;
    if (delta < registrationPeriod) {
      Serial.print("Waiting some time, because an earlier presentation already found. Delta: ");
      Serial.println(delta);
      return;
    }
  }
  auto epochTime = timeClient.getEpochTime();
  registrations[cardID] = epochTime;
  for (auto user : users) {
    for (auto id : user.identifiers) {
      if (cardID == id) {
        std::stringstream ss;
        ss << "Hello " << user.firstName << " " << user.lastName << ", you're registered!";
        Serial.println(ss.str().c_str());
        FirebaseJson json;
        json.addInt("timestamp", epochTime);
        json.addInt("userID", user.userID);
        json.addString("device", UUID.c_str());
        if (Firebase.pushJSON(firebaseData, "/registrations", json)) {
          Serial.println(firebaseData.dataPath());
          Serial.println(firebaseData.pushName());
        } else {
          Serial.println(firebaseData.errorReason());
        }
      }
    }
  }
  Serial.println("");
  Serial.flush();
}

void streamFirebaseData(FirebaseData& db, String path, StreamEventCallback callback) {
  Firebase.setStreamCallback(db, callback, streamTimeoutCallback);

  //In setup(), set the streaming path and begin stream connection
  if (!Firebase.beginStream(db, path))
  {
    //Could not begin stream connection, then print out the error detail
    Serial.println(db.errorReason());
  }
}

void usersCallback(StreamData data) {
  Serial.println("User Data...");
  Serial.println(data.dataPath());
  Serial.println(data.dataType());
  Serial.println(data.jsonData());

  DynamicJsonDocument doc(500);
  DeserializationError error = deserializeJson(doc, data.jsonData());
  if (error) {
    Serial.print("deserializeJson() failed: "); Serial.println(error.c_str());
    return;
  }
  JsonObject allUsers = doc.as<JsonObject>();
  for (JsonPair kv : allUsers) {
    User u;
    u.userID = atoi(kv.key().c_str()); // the key (user ID)
    Serial.println(u.userID);
    JsonObject userObj = kv.value().as<JsonObject>();
    
    const char* str = userObj["firstName"];
    Serial.println(str);
    u.firstName = std::string(str);
    str = userObj["lastName"];
    Serial.println(str);
    u.lastName = std::string(str);
    JsonArray idArr = userObj["identifiers"].as<JsonArray>();
    for (JsonVariant value : idArr) {
      if (value.isNull()) {
        continue;
      }
      str = value.as<char*>();
      Serial.println(str);
      u.identifiers.push_back(std::string(str));
    }
    users.push_back(u);
  }
  Serial.print("Users length: "); Serial.println(users.size());
}

void deviceCallback(StreamData data) {
  Serial.println("Device Data...");
  Serial.println(data.dataPath());
  Serial.println(data.dataType());
  Serial.println(data.jsonData());

  DynamicJsonDocument doc(100);
  DeserializationError error = deserializeJson(doc, data.jsonData());
  if (error) {
    Serial.print("deserializeJson() failed: "); Serial.println(error.c_str());
    return;
  }
  JsonObject deviceObj = doc.as<JsonObject>();
  const char* str = deviceObj["location"];
  Serial.println(str);
  currentDevice.location = std::string(str);
  currentDevice.uuid = UUID;
}

//Global function that notify when stream connection lost
//The library will resume the stream connection automatically
void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    //Stream timeout occurred
    Serial.println("Stream timeout, resume streaming...");
  }
}

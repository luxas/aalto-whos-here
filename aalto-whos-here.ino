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
   https://randomnerdtutorials.com/esp32-pwm-arduino-ide/
*/

// Install this by adding https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json to File -> Preferences -> Additional Board Manager URLs
// Then go to the board manager and add ESP32 support. We used version 1.0.4
// Include the Arduino libraries
#include <Arduino.h>

// Connect to Firebase through WiFi
#include <WiFi.h>
// Install this as Firebase ESP32 Client by Mobitz, version 3.2.1
// This also depends on curl -sSL https://github.com/mobizt/HTTPClientESP32Ex/archive/1.2.0.tar.gz | tar -xz -C ~/Arduino/libraries/
#include <FirebaseESP32.h>

// Connect to the NFC reader through SPI
#include <SPI.h>
// Installed 10.10.2019 at commit f9c62da0811d154f55e2169db4761a093c603726 using the following command:
// curl -sSL https://github.com/Seeed-Studio/PN532/archive/arduino.tar.gz | tar -xz PN532-arduino/PN532 PN532-arduino/PN532_SPI --strip-components=1 -C ~/Arduino/libraries/
// This was also a bit annoying, as we had to do a sed "s|PN532/PN532/||g" across all files downloaded
#include <PN532_SPI.h>
#include <PN532.h>

// Get the current time using some NTP servers
#include <NTPClient.h>
#include <WiFiUdp.h>

// Include secret constants
/*
There is intentionally a whitespace between # and define, there shouldn't be one in secrets.h for real:

# define FIREBASE_HOST "<domain>" //Change to your Firebase RTDB project ID e.g. Your_Project_ID.firebaseio.com
# define FIREBASE_SECRET "<secret string>" //Change to your Firebase RTDB secret password
# define WIFI_SSID "<ssid string>"
# define WIFI_PASSWORD "<ssid password>" // Can also be blank
*/
#include "secrets.h"
// Include our data types
#include "types.hpp"

#include <iomanip>
#include <sstream>
#include <map>

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
//Device currentDevice;
bool usersInitialized = false;

std::map<String, unsigned long> registrations;

unsigned long nextLoopTime = 0;

// PWM output constants
const int ledFrequency = 5000;
const int resolution = 8;
// PWM pins for the RGB LED
const int pwmPinRed = 4;
const int pwmPinGreen = 2;
const int pwmPinBlue = 0;

// Buzzer constants
const int pwmPinBuzzer = 27;
const int buzzerChannel = 6;
const int buzzerFrequency = 2000;
const unsigned long buzzTime = 500;

void setup() {
  // Open a serial connection to a computer, if connected
  Serial.begin(115200);

  // Open the SPI connection to the NFC device, using the default SS port (5 on the ESP32)
  setupNFC(nfc);

  // Connect to WiFi using the given SSID and password
  connectToWiFi(WIFI_SSID, WIFI_PASSWORD);

  // With internet connectivity set up, we can now connect to the Firebase Realtime Database
  initFirebase(firebaseData, FIREBASE_HOST, FIREBASE_SECRET);

  // TODO: Initialize the LCD

  // Setup the PWM functionality of the ESP
  ledcSetup(pwmPinRed, ledFrequency, resolution);
  ledcSetup(pwmPinGreen, ledFrequency, resolution);
  ledcSetup(pwmPinBlue, ledFrequency, resolution);
  // The channel number is the same as the pin
  ledcAttachPin(pwmPinRed, pwmPinRed);
  ledcAttachPin(pwmPinGreen, pwmPinGreen);
  ledcAttachPin(pwmPinBlue, pwmPinBlue);
  // Set the LED to be red
  rgbLED(255, 0, 0);

  // Setup the buzzer
  ledcSetup(buzzerChannel, buzzerFrequency, resolution);
  ledcAttachPin(pwmPinBuzzer, buzzerChannel);

  timeClient.begin();

  // Get initial user data from the database
  if(Firebase.getJSON(firebaseData, "/users")) {
    Serial.println("Successfully got user data.");
    handleUsers(firebaseData);
  } else {
    Serial.println("Handle users failed.");
  }
}

void rgbLED(int red, int green, int blue) {
  ledcWrite(pwmPinRed, red);
  ledcWrite(pwmPinGreen, green);
  ledcWrite(pwmPinBlue, blue);
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
  // TODO: If this takes too long, then try again
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
  // 4. Setup Firebase credential in setup()
  Firebase.begin(host, secret);

  // 5. Optional, set AP reconnection in setup()
  Firebase.reconnectWiFi(true);

  // 6. Optional, set number of auto retry when read/store data
  Firebase.setMaxRetry(db, 10);

  // 7. Optional, set number of read/store data error can be added to queue collection
  Firebase.setMaxErrorQueue(db, 30);
}

void loop() {
  // Wait until we've passed the nextLoopTime time mark.
  if (nextLoopTime < millis() && nextLoopTime != 0) {
    return;
  }

  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  
  // Wait until the async thread has initialized the users vector.
  if (!usersInitialized) {
    nextLoopTime = millis() + 2000;
    return;
  }

  // Need to reset the timer here so that it successfully starts the next round
  nextLoopTime = 0;
  
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (!success) {
    Serial.print(timeClient.getFormattedTime()); Serial.println(": no card at the moment");
    nextLoopTime = millis() + 1000;
    return;
  }
  // Display some basic information about the card
  Serial.println("Found an ISO14443A card");
  Serial.print("  Card ID: ");
  auto cardID = getCardID(uid, uidLength);
  Serial.println(cardID.c_str());
  auto it = registrations.find(cardID);
  if (it != registrations.end()) {
    auto t = registrations[cardID];
    auto delta = timeClient.getEpochTime() - t;
    if (delta < registrationPeriod) {
      Serial.print("Waiting some time, because an earlier registration already found. Delta: ");
      Serial.println(delta);
      return;
    }
    Serial.print("Delta was: "); Serial.println(delta);
  }
  auto epochTime = timeClient.getEpochTime();
  registrations[cardID] = epochTime;
  bool registrationDone = false;
  for (auto user : users) {
    for (auto id : user.identifiers) {
      if (cardID == id) {
        std::stringstream ss;
        ss << "Hello " << user.firstName << " " << user.lastName << ", you're registered!";
        Serial.println(ss.str().c_str());

        Registration r(user.userID, epochTime);
        // Start the beep feedback
        startFeedback();
        // Push the data to the database
        r.PushToFirebase(&firebaseData);
        // Stop the beep feedback
        stopFeedback();
        registrationDone = true;
      }
    }
  }
  if (!registrationDone) {
    NewCard c(epochTime, cardID);
    startFeedback();
    c.PushToFirebase(&firebaseData);
    stopFeedback();
    Serial.println("Pushed new card ID to newCards");
  }
  Serial.println("Loop done.");
  Serial.flush();
}

void startFeedback() {
  // Set the LED to green for a while
  rgbLED(0, 255, 0);
  // Play buzzer sound
  ledcWrite(buzzerChannel, 255);
  ledcWriteTone(buzzerChannel, 1500);
  // TODO: Display the welcome message on the LCD screen
}

void stopFeedback() {
  ledcWrite(buzzerChannel, 0);
  ledcWriteTone(buzzerChannel, 0);
  delay(50);
  rgbLED(0, 0, 255);
}

String getCardID(uint8_t uid[], uint8_t uidLength) {
  std::stringstream ss;
  for (uint8_t i = 0; i < uidLength; i++) {
    if (i > 0) {
      ss << ":";
    }
    char buf[2];
    sprintf(buf, "%02hhx", uid[i]);
    ss << buf;
  }
  return ss.str().c_str();
}

void handleUsers(FirebaseData& data) {
  Serial.println("User Data...");
  
  users.clear();

  FirebaseJson* json = &data.jsonObject();
  if (json == nullptr) {
    Serial.println("json is nullptr!");
    return;
  }
  String jsonStr = "";
  json->toString(jsonStr);
  Serial.println(jsonStr);

  size_t len = json->iteratorBegin();
  String key, value = "";
  String firstName, lastName = "";
  int type = 0;
  for (size_t i = 0; i < len; i++)
  {
    json->iteratorGet(i, type, key, value);
    Serial.print("key is this for index "); Serial.println(i); Serial.println(key); Serial.println(value);
    FirebaseJsonData tmpObj;
    json->get(tmpObj, key + "/firstName");
    firstName = tmpObj.stringValue;
    Serial.println(firstName);
    json->get(tmpObj, key + "/lastName");
    lastName = tmpObj.stringValue;
    Serial.println(lastName);
    String fullName = firstName + " " + lastName;
    json->get(tmpObj, key + "/identifiers");
    FirebaseJsonArray arr;
    tmpObj.getArray(arr);
    std::vector<String> identifiers;
    for (size_t j = 0; j < arr.size(); j++) {
      arr.get(tmpObj, j);
      identifiers.push_back(tmpObj.stringValue);
      Serial.println(tmpObj.stringValue);
    }
    User u(atoi(key.c_str()), firstName, lastName, identifiers);
    users.push_back(u);
    Serial.println("next user.");
  }
  //Clear all list to free memory
  json->iteratorEnd();

  // Similar setup with ArduinoJson, which we didn't use in the end.
  /*DynamicJsonDocument doc(jsonStr.length() * 3);
  DeserializationError error = deserializeJson(doc, jsonStr);
  if (error) {
    Serial.print("deserializeJson() failed: "); Serial.println(error.c_str());
    return;
  }
  // TODO: Use FirebaseJSON consistently here for parsing the data, instead of this extra library
  users.clear();
  JsonObject allUsers = doc.as<JsonObject>();
  for (JsonPair kv : allUsers) {
    JsonObject userObj = kv.value().as<JsonObject>();
    User u(atoi(kv.key().c_str()), userObj);
    users.push_back(u);

    Serial.println(u.userID);
  }*/
  usersInitialized = true;
  rgbLED(0, 0, 255);
  Serial.print("Users length: "); Serial.println(users.size());
}

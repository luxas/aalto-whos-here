/*
 * Good links and resources:
 * https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
 * https://blog.atlasrfidstore.com/near-field-communication-infographic
 * https://randomnerdtutorials.com/security-access-using-mfrc522-rfid-reader-with-arduino/
 * https://www.instructables.com/id/ESP32-With-RFID-Access-Control/
 * SPI pins on ESP32: https://github.com/espressif/arduino-esp32/blob/master/variants/esp32/pins_arduino.h#L20-L23
 * https://seeeddoc.github.io/NFC_Shield_V1.0/
 * https://github.com/elechouse/PN532/tree/PN532_HSU
 * http://www.elechouse.com/elechouse/index.php?main_page=product_info&cPath=90_93&products_id=2242
 * 
 */


//1. Download  HTTPClientESP32Ex library above and add to Arduino library

//2. Include Firebase ESP32 library (this library)
#include <WiFi.h>
#include <FirebaseESP32.h>

#define FIREBASE_HOST "" //Change to your Firebase RTDB project ID e.g. Your_Project_ID.firebaseio.com
#define FIREBASE_SECRET "" //Change to your Firebase RTDB secret password
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

PN532_SPI pn532spi(SPI, SS);
PN532 nfc(pn532spi);

//3. Declare the Firebase Data object in global scope
FirebaseData firebaseData;

void setup() {
  Serial.begin(115200);

  setupNFC();

  connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
  
  initFirebase(FIREBASE_HOST, FIREBASE_SECRET);

  streamFirebaseData(firebaseData, "/", streamCallback);
}

void setupNFC() {
  Serial.println("Looking for PN532...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop() {
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t currentblock;                     // Counter to keep track of which block we're on
  bool authenticated = false;               // Flag to indicate if the sector is authenticated
  uint8_t data[16];                         // Array to store block data during reads
  
  // Keyb on NDEF and Mifare Classic should be the same
  uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (!success) {
    Serial.println("no card at the moment");
    sleep(1);
    Serial.println("return");
    return;
  }
  // Display some basic information about the card
  Serial.println("Found an ISO14443A card");
  Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
  Serial.print("  UID Value: ");
  for (uint8_t i = 0; i < uidLength; i++) {
    Serial.print(uid[i], HEX);
    Serial.print(' ');
  }
  Serial.println("");

  if (uidLength == 4 || uidLength == 7)
  {
    // We probably have a Mifare Classic card ...
    Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

    // Now we try to go through all 16 sectors (each having 4 blocks)
    // authenticating each sector, and then dumping the blocks
    for (currentblock = 0; currentblock < 64; currentblock++)
    {
      // Check if this is a new block so that we can reauthenticate
      if (nfc.mifareclassic_IsFirstBlock(currentblock)) authenticated = false;

      // If the sector hasn't been authenticated, do so first
      if (!authenticated)
      {
        // Starting of a new sector ... try to to authenticate
        Serial.print("------------------------Sector ");Serial.print(currentblock/4, DEC);Serial.println("-------------------------");
        if (currentblock == 0)
        {
            // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
            // or 0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 for NDEF formatted cards using key a,
            // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
            success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
        }
        else
        {
            // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
            // or 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7 for NDEF formatted cards using key a,
            // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
            success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
        }
        if (success)
        {
          authenticated = true;
        }
        else
        {
          Serial.println("Authentication error");
        }
      }
      // If we're still not authenticated just skip the block
      if (!authenticated)
      {
        Serial.print("Block ");Serial.print(currentblock, DEC);Serial.println(" unable to authenticate");
      }
      else
      {
        // Authenticated ... we should be able to read the block now
        // Dump the data into the 'data' array
        success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
        if (success)
        {
          // Read successful
          Serial.print("Block ");Serial.print(currentblock, DEC);
          if (currentblock < 10)
          {
            Serial.print("  ");
          }
          else
          {
            Serial.print(" ");
          }
          // Dump the raw data
          nfc.PrintHexChar(data, 16);
        }
        else
        {
          // Oops ... something happened
          Serial.print("Block ");Serial.print(currentblock, DEC);
          Serial.println(" unable to read this block");
        }
      }
    }
  }
  else
  {
    Serial.println("Ooops ... this doesn't seem to be a Mifare Classic card!");
  }
  Serial.flush();
}

/*
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance. 

void setup() {
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  mfrc522.PCD_DumpVersionToSerial();   // Initiate MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println();
}
void loop() 
{
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == "BD 31 15 2B") //change here the UID of the card/cards that you want to give access
  {
    Serial.println("Authorized access");
    Serial.println();
    delay(3000);
  }
 
 else   {
    Serial.println(" Access denied");
    delay(3000);
  }
}*/

void streamFirebaseData(FirebaseData& db, String path, StreamEventCallback callback) {
  Firebase.setStreamCallback(db, callback, streamTimeoutCallback);
  
  //In setup(), set the streaming path to "/" and begin stream connection
  if (!Firebase.beginStream(db, path))
  {
    //Could not begin stream connection, then print out the error detail
    Serial.println(db.errorReason());
  }
}

//Global function that handle stream data
void streamCallback(StreamData data) {
  //Print out all information
  Serial.println("Stream Data...");
  Serial.println(data.streamPath());
  Serial.println(data.dataPath());
  Serial.println(data.dataType());

  //Print out value
  //Stream data can be many types which can be determined from function dataType
  if (data.dataType() == "int")
    Serial.println(data.intData());
  else if (data.dataType() == "float")
    Serial.println(data.floatData(), 5);
  else if (data.dataType() == "double")
    printf("%.9lf\n", data.doubleData());
  else if (data.dataType() == "boolean")
    Serial.println(data.boolData() == 1 ? "true" : "false");
  else if (data.dataType() == "string")
    Serial.println(data.stringData());
  else if (data.dataType() == "json")
    Serial.println(data.jsonData());
}

//Global function that notify when stream connection lost
//The library will resume the stream connection automatically
void streamTimeoutCallback(bool timeout) {
  if(timeout){
    //Stream timeout occurred
    Serial.println("Stream timeout, resume streaming...");
  }
}

void initFirebase(String host, String secret) {
  // put your setup code here, to run once:
  //4. Setup Firebase credential in setup()
  Firebase.begin(host, secret);
  
  //5. Optional, set AP reconnection in setup()
  Firebase.reconnectWiFi(true);
  
  //6. Optional, set number of auto retry when read/store data
  Firebase.setMaxRetry(firebaseData, 3);
  
  //7. Optional, set number of read/store data error can be added to queue collection
  Firebase.setMaxErrorQueue(firebaseData, 30);
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

#define SS_PIN D4
#define RST_PIN D3
#define RELAY_PIN D2 // Pin connected to relay

#include <SPI.h>
#include <MFRC522.h>
#include "connection.h" // Assumes this file contains connectToWiFi() and connectToFirebase() functions
#include <FirebaseESP8266.h> // Include the Firebase library for ESP8266

FirebaseData firebaseData; // Create an instance of FirebaseData
MFRC522 mfrc522(SS_PIN, RST_PIN);
int statuss = 0;

// Forward declarations for the Firebase callback functions
void firebaseStreamCallback(StreamData data);
void firebaseStreamTimeoutCallback(bool timeout);

void setup() 
{
  connectToWiFi();
  connectToFirebase();
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Default relay state

  // Set up Firebase listener for /LockStatus
  if (!Firebase.beginStream(firebaseData, "/LockStatus")) {
    Serial.println("Failed to begin Firebase stream:");
    Serial.println(firebaseData.errorReason());
  } else {
    Serial.println("Firebase stream started successfully.");
    Firebase.setStreamCallback(firebaseData, firebaseStreamCallback, firebaseStreamTimeoutCallback);
  }
}

void loop() 
{
  // Check if the Firebase stream is still active
  if (!Firebase.readStream(firebaseData)) {
    Serial.println("Stream read failed:");
    Serial.println(firebaseData.errorReason());
    // Try to restart the stream if there’s an error
    if (!Firebase.beginStream(firebaseData, "/LockStatus")) {
      Serial.println("Failed to restart Firebase stream:");
      Serial.println(firebaseData.errorReason());
    } else {
      Serial.println("Firebase stream restarted successfully.");
    }
  }

  // RFID card access control
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  Serial.println();
  Serial.print(" UID tag :");
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  Serial.println();

  if (content.substring(1) == "14 04 2D A7" || content.substring(1) == "C3 73 30 E2") 
  {
    Serial.println(" Access Granted");
    digitalWrite(RELAY_PIN, LOW);

    if (Firebase.setInt(firebaseData, "/LockStatus", 1)) {
      Serial.println("Firebase updated: LockStatus set to 1");
    } else {
      Serial.print("Firebase update failed: ");
      Serial.println(firebaseData.errorReason());
    }

    delay(2000);

    if (Firebase.setInt(firebaseData, "/LockStatus", 0)) {
      Serial.println("Firebase updated: LockStatus set to 0");
    } else {
      Serial.print("Firebase update failed: ");
      Serial.println(firebaseData.errorReason());
    }

    digitalWrite(RELAY_PIN, HIGH);
    Serial.println(" Have FUN ");
    Serial.println();
    statuss = 1;
  } 
  else 
  {
    Serial.println(" Access Denied ");
    delay(3000);
  }
}

// Firebase stream callback function to listen for changes in /LockStatus
void firebaseStreamCallback(StreamData data)
{
  if (data.streamPath() == "/LockStatus" && data.dataType() == "int") {
    int lockStatus = data.intData();
    if (lockStatus == 1) {
      digitalWrite(RELAY_PIN, LOW); // Activate relay when /LockStatus is 1
      Serial.println("Firebase update: Relay set to LOW (ON) based on LockStatus = 1");
    } else if (lockStatus == 0) {
      digitalWrite(RELAY_PIN, HIGH); // Deactivate relay when /LockStatus is 0
      Serial.println("Firebase update: Relay set to HIGH (OFF) based on LockStatus = 0");
    }
  }
}

// Firebase timeout callback (optional, for debug)
void firebaseStreamTimeoutCallback(bool timeout)
{
  if (timeout) {
    Serial.println("Firebase stream timeout, resuming...");
    Firebase.beginStream(firebaseData, "/LockStatus");
  }
}

// Undefine HIGH before including Keypad.h to avoid conflicts
#undef HIGH
#include <SPI.h>
#include <Keypad.h>  // Include Keypad library after undefining HIGH
#include "connection.h"  // Assumes this file contains connectToWiFi() and connectToFirebase() functions
#include <FirebaseESP8266.h>  // Include Firebase ESP8266 library
#define HIGH 0x1  // Redefine HIGH back to its correct value for general use

FirebaseData firebaseData;  // Create an instance of FirebaseData

const byte ROWS = 4;  // Four rows
const byte COLS = 3;  // Three columns

// Define the symbols on the buttons of the keypad (for a 4x3 keypad)
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// Define ESP8266 GPIO pins for rows and columns
byte rowPins[ROWS] = {D1, D2, D5, D6};  // GPIO5, GPIO4, GPIO12, GPIO14 (D1, D2, D5, D6 on ESP8266)
byte colPins[COLS] = {D3, D4, D7};     // GPIO0, GPIO2, GPIO13 (D3, D4, D7 on ESP8266)

// Initialize an instance of the Keypad class
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// Define the static passkey to check against (for example, "1234")
String savedPasskey = "1234";
String enteredPasskey = "";  // To store the entered passkey

void setup() {
  connectToWiFi();
  connectToFirebase();
  
  Serial.begin(9600);  // Start serial communication at 9600 baud
  Serial.println("Enter passkey:");
}

void loop() {
  char customKey = customKeypad.getKey();  // Get the key pressed
  
  if (customKey) {
    // Print the pressed key
    Serial.print("Key Pressed: ");
    Serial.println(customKey);

    // If user presses the '#' key, check the entered passkey
    if (customKey == '#') {
      checkPasskey();
      enteredPasskey = "";  // Clear entered passkey after checking
    } 
    // If the '*' key is pressed, clear the entered passkey
    else if (customKey == '*') {
      enteredPasskey = "";  // Clear entered passkey
      Serial.println("Passkey cleared.");
    } 
    // Add the pressed key to the entered passkey
    else {
      enteredPasskey += customKey;
    }
  }
}

void checkPasskey() {
  // Compare entered passkey with saved passkey
  if (enteredPasskey == savedPasskey) {
    Serial.println("Access Granted!");  // If passkey is correct

    // Set LockStatus to 1 in Firebase
    if (Firebase.setInt(firebaseData, "/LockStatus", 1)) {
      Serial.println("Firebase updated: LockStatus set to 1");
    } else {
      Serial.print("Firebase update failed: ");
      Serial.println(firebaseData.errorReason());
    }

    // Wait for 2 seconds
    delay(2000);

    // Set LockStatus back to 0 in Firebase after 2 seconds
    if (Firebase.setInt(firebaseData, "/LockStatus", 0)) {
      Serial.println("Firebase updated: LockStatus set to 0");
    } else {
      Serial.print("Firebase update failed: ");
      Serial.println(firebaseData.errorReason());
    }

  } else {
    Serial.println("Incorrect Passkey!");  // If passkey is incorrect
  }
}

/*
  Basic BLE OTA Update Example
  
  This example demonstrates how to use the BLE OTA Update library
  with default settings and basic callbacks.
  
  The ESP32 will create a BLE service that allows firmware updates
  over Bluetooth Low Energy.
*/

#include <BLEOtaUpdate.h>

// Create BLE OTA instance with default UUIDs
BLEOtaUpdate bleOta;

// LED pin for status indication
const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  
  // Configure LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("Starting BLE OTA Update Example...");
  
  // Set up callbacks
  bleOta.setOtaProgressCallback(onOtaProgress);
  bleOta.setOtaStatusCallback(onOtaStatus);
  bleOta.setCommandCallback(onCommand);
  bleOta.setConnectionCallback(onConnection);
  
  // Start BLE OTA service
  bleOta.begin("ESP32-OTA-Device");
  
  Serial.println("BLE OTA service is ready!");
  Serial.println("Connect to 'ESP32-OTA-Device' to perform firmware updates");
}

void loop() {
  // Main loop - all work is done in BLE callbacks
  delay(100);
  
  // Blink LED to show the device is running
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 2000) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
}

// OTA Progress Callback
void onOtaProgress(uint32_t received, uint32_t total, uint8_t percentage) {
  Serial.printf("OTA Progress: %d%% (%u/%u bytes)\n", percentage, received, total);
  
  // Visual feedback with LED
  if (percentage % 10 == 0) {
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
  }
}

// OTA Status Callback
void onOtaStatus(OtaStatus status, const char* message) {
  Serial.printf("OTA Status: %s\n", message);
  
  switch (status) {
    case OtaStatus::IDLE:
      digitalWrite(LED_PIN, LOW);
      break;
    case OtaStatus::RECEIVING:
      digitalWrite(LED_PIN, HIGH);
      break;
    case OtaStatus::COMPLETED:
      // LED will stay on until reboot
      break;
    case OtaStatus::ERROR:
      // Blink rapidly to indicate error
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
      }
      break;
    case OtaStatus::ABORTED:
      digitalWrite(LED_PIN, LOW);
      break;
  }
}

// Command Callback
void onCommand(const String& command) {
  Serial.printf("Received command: %s\n", command.c_str());
  
  // Handle custom commands here
  if (command == "LED_ON") {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED turned ON");
  } else if (command == "LED_OFF") {
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED turned OFF");
  } else if (command == "STATUS") {
    Serial.printf("Device Status: Connected=%s, UpdateInProgress=%s\n", 
                  bleOta.isConnected() ? "true" : "false",
                  bleOta.isUpdateInProgress() ? "true" : "false");
  }
}

// Connection Callback
void onConnection(bool connected) {
  if (connected) {
    Serial.println("BLE client connected");
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("BLE client disconnected");
    digitalWrite(LED_PIN, LOW);
  }
}

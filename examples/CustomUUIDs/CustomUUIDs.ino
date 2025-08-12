/*
  Custom UUIDs BLE OTA Update Example
  
  This example demonstrates how to use the BLE OTA Update library
  with custom service and characteristic UUIDs.
  
  This is useful when you want to:
  - Use your own UUIDs for branding
  - Avoid conflicts with other BLE services
  - Create multiple OTA services on the same device
*/

#include <BLEOtaUpdate.h>

// Custom UUIDs - you can generate your own using online UUID generators
const char* CUSTOM_SERVICE_UUID = "12345678-1234-1234-1234-123456789ABC";
const char* CUSTOM_OTA_CHAR_UUID = "87654321-4321-4321-4321-CBA987654321";
const char* CUSTOM_COMMAND_CHAR_UUID = "11111111-2222-3333-4444-555555555555";
const char* CUSTOM_STATUS_CHAR_UUID = "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE";

// Create BLE OTA instance with custom UUIDs
BLEOtaUpdate bleOta(CUSTOM_SERVICE_UUID, CUSTOM_OTA_CHAR_UUID, CUSTOM_COMMAND_CHAR_UUID, CUSTOM_STATUS_CHAR_UUID);

// LED pin for status indication
const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  
  // Configure LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("Starting Custom UUIDs BLE OTA Update Example...");
  Serial.println("Using custom UUIDs:");
  Serial.printf("Service UUID: %s\n", CUSTOM_SERVICE_UUID);
  Serial.printf("OTA Characteristic UUID: %s\n", CUSTOM_OTA_CHAR_UUID);
  Serial.printf("Command Characteristic UUID: %s\n", CUSTOM_COMMAND_CHAR_UUID);
  Serial.printf("Status Characteristic UUID: %s\n", CUSTOM_STATUS_CHAR_UUID);
  
  // Set up callbacks
  bleOta.setOtaProgressCallback(onOtaProgress);
  bleOta.setOtaStatusCallback(onOtaStatus);
  bleOta.setCommandCallback(onCommand);
  bleOta.setConnectionCallback(onConnection);
  
  // Start BLE OTA service with custom UUIDs
  bleOta.begin("ESP32-Custom-OTA");
  
  Serial.println("Custom BLE OTA service is ready!");
  Serial.println("Connect to 'ESP32-Custom-OTA' to perform firmware updates");
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
  } else if (command == "UUID_INFO") {
    Serial.println("Custom UUIDs in use:");
    Serial.printf("Service: %s\n", CUSTOM_SERVICE_UUID);
    Serial.printf("OTA Char: %s\n", CUSTOM_OTA_CHAR_UUID);
    Serial.printf("Command Char: %s\n", CUSTOM_COMMAND_CHAR_UUID);
    Serial.printf("Status Char: %s\n", CUSTOM_STATUS_CHAR_UUID);
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

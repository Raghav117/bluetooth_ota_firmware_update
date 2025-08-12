/*
  Advanced BLE OTA Update Example
  
  This example demonstrates advanced features of the BLE OTA Update library:
  - Dynamic UUID configuration
  - Advanced status monitoring
  - Custom packet size configuration
  - Multiple command handling
  - Status reporting
*/

#include <BLEOtaUpdate.h>

// Create BLE OTA instance
BLEOtaUpdate bleOta;

// LED pins for different status indications
const int STATUS_LED = 2;
const int PROGRESS_LED = 4;
const int ERROR_LED = 5;

// Status variables
unsigned long lastStatusReport = 0;
unsigned long connectionTime = 0;
bool wasConnected = false;

void setup() {
  Serial.begin(115200);
  
  // Configure LEDs
  pinMode(STATUS_LED, OUTPUT);
  pinMode(PROGRESS_LED, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);
  
  // Turn off all LEDs initially
  digitalWrite(STATUS_LED, LOW);
  digitalWrite(PROGRESS_LED, LOW);
  digitalWrite(ERROR_LED, LOW);
  
  Serial.println("Starting Advanced BLE OTA Update Example...");
  
  // Configure advanced settings
  bleOta.setMaxPacketSize(1024);  // Larger packet size for faster transfers
  bleOta.setUpdateBufferSize(8192); // Larger buffer for better performance
  
  // Set up callbacks
  bleOta.setOtaProgressCallback(onOtaProgress);
  bleOta.setOtaStatusCallback(onOtaStatus);
  bleOta.setCommandCallback(onCommand);
  bleOta.setConnectionCallback(onConnection);
  
  // Start BLE OTA service
  bleOta.begin("ESP32-Advanced-OTA");
  
  Serial.println("Advanced BLE OTA service is ready!");
  Serial.println("Available commands:");
  Serial.println("  LED_ON/LED_OFF - Control status LED");
  Serial.println("  STATUS - Get device status");
  Serial.println("  CONFIG - Get current configuration");
  Serial.println("  RESTART - Restart BLE service");
  Serial.println("  ABORT - Abort current update");
}

void loop() {
  // Main loop with advanced monitoring
  delay(100);
  
  // Periodic status reporting
  if (millis() - lastStatusReport > 5000) { // Every 5 seconds
    reportStatus();
    lastStatusReport = millis();
  }
  
  // Connection time monitoring
  if (bleOta.isConnected() && !wasConnected) {
    connectionTime = millis();
    wasConnected = true;
  } else if (!bleOta.isConnected() && wasConnected) {
    wasConnected = false;
    Serial.printf("Connection duration: %lu seconds\n", (millis() - connectionTime) / 1000);
  }
  
  // Visual status indication
  updateStatusLEDs();
}

// Advanced OTA Progress Callback
void onOtaProgress(uint32_t received, uint32_t total, uint8_t percentage) {
  Serial.printf("OTA Progress: %d%% (%u/%u bytes)\n", percentage, received, total);
  
  // Visual progress indication
  if (percentage % 5 == 0) { // Every 5%
    digitalWrite(PROGRESS_LED, HIGH);
    delay(100);
    digitalWrite(PROGRESS_LED, LOW);
  }
  
  // Send progress to client
  bleOta.sendProgress(received, total);
}

// Advanced OTA Status Callback
void onOtaStatus(OtaStatus status, const char* message) {
  Serial.printf("OTA Status: %s\n", message);
  
  switch (status) {
    case OtaStatus::IDLE:
      digitalWrite(STATUS_LED, LOW);
      digitalWrite(PROGRESS_LED, LOW);
      digitalWrite(ERROR_LED, LOW);
      break;
      
    case OtaStatus::RECEIVING:
      digitalWrite(STATUS_LED, HIGH);
      digitalWrite(ERROR_LED, LOW);
      break;
      
    case OtaStatus::COMPLETED:
      digitalWrite(STATUS_LED, HIGH);
      digitalWrite(PROGRESS_LED, HIGH);
      digitalWrite(ERROR_LED, LOW);
      break;
      
    case OtaStatus::ERROR:
      digitalWrite(ERROR_LED, HIGH);
      digitalWrite(STATUS_LED, LOW);
      digitalWrite(PROGRESS_LED, LOW);
      // Blink error LED
      for (int i = 0; i < 3; i++) {
        digitalWrite(ERROR_LED, HIGH);
        delay(200);
        digitalWrite(ERROR_LED, LOW);
        delay(200);
      }
      break;
      
    case OtaStatus::ABORTED:
      digitalWrite(ERROR_LED, HIGH);
      delay(1000);
      digitalWrite(ERROR_LED, LOW);
      break;
  }
  
  // Send status to client
  bleOta.sendStatus(message);
}

// Advanced Command Callback
void onCommand(const String& command) {
  Serial.printf("Received command: %s\n", command.c_str());
  
  if (command == "LED_ON") {
    digitalWrite(STATUS_LED, HIGH);
    bleOta.sendStatus("LED turned ON");
    
  } else if (command == "LED_OFF") {
    digitalWrite(STATUS_LED, LOW);
    bleOta.sendStatus("LED turned OFF");
    
  } else if (command == "STATUS") {
    String status = "Connected: " + String(bleOta.isConnected() ? "true" : "false");
    status += ", UpdateInProgress: " + String(bleOta.isUpdateInProgress() ? "true" : "false");
    status += ", Progress: " + String(bleOta.getUpdatePercentage()) + "%";
    bleOta.sendStatus(status);
    
  } else if (command == "CONFIG") {
    String config = "MaxPacketSize: 1024, BufferSize: 8192";
    bleOta.sendStatus(config);
    
  } else if (command == "RESTART") {
    bleOta.restart();
    bleOta.sendStatus("BLE service restarted");
    
  } else if (command == "ABORT") {
    if (bleOta.isUpdateInProgress()) {
      bleOta.abortUpdate();
      bleOta.sendStatus("Update aborted");
    } else {
      bleOta.sendStatus("No update in progress");
    }
    
  } else if (command == "UPTIME") {
    String uptime = "Uptime: " + String(millis() / 1000) + " seconds";
    bleOta.sendStatus(uptime);
    
  } else if (command == "MEMORY") {
    String memory = "Free heap: " + String(ESP.getFreeHeap()) + " bytes";
    bleOta.sendStatus(memory);
    
  } else {
    bleOta.sendStatus("Unknown command: " + command);
  }
}

// Advanced Connection Callback
void onConnection(bool connected) {
  if (connected) {
    Serial.println("BLE client connected");
    digitalWrite(STATUS_LED, HIGH);
    bleOta.sendStatus("Welcome! Device ready for OTA updates");
  } else {
    Serial.println("BLE client disconnected");
    digitalWrite(STATUS_LED, LOW);
    digitalWrite(PROGRESS_LED, LOW);
  }
}

// Helper functions
void reportStatus() {
  if (bleOta.isConnected()) {
    String status = "Heartbeat - Uptime: " + String(millis() / 1000) + "s";
    status += ", Free heap: " + String(ESP.getFreeHeap()) + "B";
    if (bleOta.isUpdateInProgress()) {
      status += ", OTA: " + String(bleOta.getUpdatePercentage()) + "%";
    }
    bleOta.sendStatus(status);
  }
}

void updateStatusLEDs() {
  // Blink status LED when connected but idle
  static unsigned long lastBlink = 0;
  if (bleOta.isConnected() && !bleOta.isUpdateInProgress() && millis() - lastBlink > 1000) {
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    lastBlink = millis();
  }
}

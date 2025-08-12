/*
  Robot BLE OTA Update Example
  
  This example integrates the BLE OTA Update library with robot control functionality.
  It demonstrates how to use OTA updates while maintaining robot control capabilities.
  
  Based on the original robot control code with added BLE OTA functionality.
*/

#include <BLEOtaUpdate.h>

// Custom UUIDs from original BLEOtaHandler
const char* CUSTOM_SERVICE_UUID = "12233445-D481-49B0-BE32-8CE24AC0F09C";
const char* CUSTOM_OTA_CHAR_UUID = "12233446-D481-49B0-BE32-8CE24AC0F09C";
const char* CUSTOM_COMMAND_CHAR_UUID = "12233447-D481-49B0-BE32-8CE24AC0F09C";
const char* CUSTOM_STATUS_CHAR_UUID = "12233448-D481-49B0-BE32-8CE24AC0F09C";

// Create BLE OTA instance with custom UUIDs
BLEOtaUpdate bleOta(CUSTOM_SERVICE_UUID, CUSTOM_OTA_CHAR_UUID, CUSTOM_COMMAND_CHAR_UUID, CUSTOM_STATUS_CHAR_UUID);

// Motor Speed Control Pins (PWM)
#define LEFT_MOTOR_ENABLE 13
#define RIGHT_MOTOR_ENABLE 15

// PWM Configuration
#define PWM_FREQUENCY 5000
#define PWM_CHANNEL_LEFT 0
#define PWM_CHANNEL_RIGHT 1
#define PWM_RESOLUTION 8

// Motor Direction Pins
#define LEFT_MOTOR_IN1 26
#define LEFT_MOTOR_IN2 25
#define RIGHT_MOTOR_IN3 12
#define RIGHT_MOTOR_IN4 14

// LED pin for status indication
#define LED_PIN 2

// Robot state
int speed = 200;
bool robotEnabled = true;

void setup() {
  Serial.begin(115200);
  
  // Configure motor pins
  pinMode(LEFT_MOTOR_IN1, OUTPUT);
  pinMode(LEFT_MOTOR_IN2, OUTPUT);
  pinMode(RIGHT_MOTOR_IN3, OUTPUT);
  pinMode(RIGHT_MOTOR_IN4, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Setup PWM
  setupPWM();
  
  // Stop motors initially
  stopMotors();
  
  Serial.println("Starting Robot BLE OTA Example with Custom UUIDs...");
  Serial.println("Using custom UUIDs:");
  Serial.printf("Service UUID: %s\n", CUSTOM_SERVICE_UUID);
  Serial.printf("OTA Characteristic UUID: %s\n", CUSTOM_OTA_CHAR_UUID);
  Serial.printf("Command Characteristic UUID: %s\n", CUSTOM_COMMAND_CHAR_UUID);
  Serial.printf("Status Characteristic UUID: %s\n", CUSTOM_STATUS_CHAR_UUID);
  
  // Set up BLE OTA callbacks
  bleOta.setOtaProgressCallback(onOtaProgress);
  bleOta.setOtaStatusCallback(onOtaStatus);
  bleOta.setCommandCallback(onCommand);
  bleOta.setConnectionCallback(onConnection);
  
  // Start BLE OTA service
  bleOta.begin("Robot-ESP32");
  
  Serial.println("Robot BLE OTA service is ready!");
  Serial.println("Available commands:");
  Serial.println("  UP/DOWN/LEFT/RIGHT/STOP - Robot movement");
  Serial.println("  SPEED_XXX - Set speed (0-255)");
  Serial.println("  ENABLE/DISABLE - Enable/disable robot");
  Serial.println("  STATUS - Get robot status");
}

void loop() {
  // Main loop - all BLE work is done in callbacks
  delay(100);
  
  // Blink LED to show the device is running
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 2000) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
}

// BLE OTA Callbacks
void onOtaProgress(uint32_t received, uint32_t total, uint8_t percentage) {
  Serial.printf("OTA Progress: %d%% (%u/%u bytes)\n", percentage, received, total);
  
  // Visual feedback with LED
  if (percentage % 10 == 0) {
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
  }
}

void onOtaStatus(OtaStatus status, const char* message) {
  Serial.printf("OTA Status: %s\n", message);
  
  switch (status) {
    case OtaStatus::IDLE:
      digitalWrite(LED_PIN, LOW);
      break;
    case OtaStatus::RECEIVING:
      digitalWrite(LED_PIN, HIGH);
      // Stop robot during update for safety
      if (robotEnabled) {
        stopMotors();
      }
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

void onCommand(const String& command) {
  Serial.printf("Received command: %s\n", command.c_str());
  
  if (!robotEnabled && !command.startsWith("ENABLE")) {
    bleOta.sendStatus("Robot disabled. Send ENABLE to enable.");
    return;
  }
  
  // Robot movement commands
  if (command == "UP") {
    moveForward();
    bleOta.sendStatus("Moving forward");
  } else if (command == "DOWN") {
    moveBackward();
    bleOta.sendStatus("Moving backward");
  } else if (command == "LEFT") {
    turnLeft();
    bleOta.sendStatus("Turning left");
  } else if (command == "RIGHT") {
    turnRight();
    bleOta.sendStatus("Turning right");
  } else if (command == "STOP") {
    stopMotors();
    bleOta.sendStatus("Stopped");
  }
  // Speed control
  else if (command.startsWith("SPEED_")) {
    String speedValue = command.substring(6);
    speed = speedValue.toInt();
    speed = constrain(speed, 0, 255);
    bleOta.sendStatus("Speed set to " + String(speed));
  }
  // Robot control
  else if (command == "ENABLE") {
    robotEnabled = true;
    bleOta.sendStatus("Robot enabled");
  } else if (command == "DISABLE") {
    robotEnabled = false;
    stopMotors();
    bleOta.sendStatus("Robot disabled");
  }
  // Status commands
  else if (command == "STATUS") {
    String status = "Robot Status: ";
    status += robotEnabled ? "Enabled" : "Disabled";
    status += ", Speed: " + String(speed);
    status += ", Connected: " + String(bleOta.isConnected() ? "true" : "false");
    status += ", UpdateInProgress: " + String(bleOta.isUpdateInProgress() ? "true" : "false");
    bleOta.sendStatus(status);
  } else if (command == "UPTIME") {
    String uptime = "Uptime: " + String(millis() / 1000) + " seconds";
    bleOta.sendStatus(uptime);
  } else if (command == "MEMORY") {
    String memory = "Free heap: " + String(ESP.getFreeHeap()) + " bytes";
    bleOta.sendStatus(memory);
  } else if (command == "UUID_INFO") {
    String uuidInfo = "Custom UUIDs in use:\n";
    uuidInfo += "Service: " + String(CUSTOM_SERVICE_UUID) + "\n";
    uuidInfo += "OTA Char: " + String(CUSTOM_OTA_CHAR_UUID) + "\n";
    uuidInfo += "Command Char: " + String(CUSTOM_COMMAND_CHAR_UUID) + "\n";
    uuidInfo += "Status Char: " + String(CUSTOM_STATUS_CHAR_UUID);
    bleOta.sendStatus(uuidInfo);
  } else {
    bleOta.sendStatus("Unknown command: " + command);
  }
}

void onConnection(bool connected) {
  if (connected) {
    Serial.println("BLE client connected");
    digitalWrite(LED_PIN, HIGH);
    bleOta.sendStatus("Robot connected and ready!");
  } else {
    Serial.println("BLE client disconnected");
    digitalWrite(LED_PIN, LOW);
  }
}

// Robot movement functions
void moveForward() {
  if (!robotEnabled) return;
  
  Serial.println("Action: Moving Forward");
  digitalWrite(LEFT_MOTOR_IN1, HIGH);
  digitalWrite(LEFT_MOTOR_IN2, LOW);
  digitalWrite(RIGHT_MOTOR_IN3, HIGH);
  digitalWrite(RIGHT_MOTOR_IN4, LOW);
  
  ledcWrite(PWM_CHANNEL_LEFT, speed);
  ledcWrite(PWM_CHANNEL_RIGHT, speed);
}

void moveBackward() {
  if (!robotEnabled) return;
  
  Serial.println("Action: Moving Backward");
  digitalWrite(LEFT_MOTOR_IN1, LOW);
  digitalWrite(LEFT_MOTOR_IN2, HIGH);
  digitalWrite(RIGHT_MOTOR_IN3, LOW);
  digitalWrite(RIGHT_MOTOR_IN4, HIGH);
  
  ledcWrite(PWM_CHANNEL_LEFT, speed);
  ledcWrite(PWM_CHANNEL_RIGHT, speed);
}

void turnLeft() {
  if (!robotEnabled) return;
  
  Serial.println("Action: Turning Left");
  digitalWrite(LEFT_MOTOR_IN1, LOW);
  digitalWrite(LEFT_MOTOR_IN2, HIGH);
  digitalWrite(RIGHT_MOTOR_IN3, HIGH);
  digitalWrite(RIGHT_MOTOR_IN4, LOW);
  
  ledcWrite(PWM_CHANNEL_LEFT, speed);
  ledcWrite(PWM_CHANNEL_RIGHT, speed);
}

void turnRight() {
  if (!robotEnabled) return;
  
  Serial.println("Action: Turning Right");
  digitalWrite(LEFT_MOTOR_IN1, HIGH);
  digitalWrite(LEFT_MOTOR_IN2, LOW);
  digitalWrite(RIGHT_MOTOR_IN3, LOW);
  digitalWrite(RIGHT_MOTOR_IN4, HIGH);
  
  ledcWrite(PWM_CHANNEL_LEFT, speed);
  ledcWrite(PWM_CHANNEL_RIGHT, speed);
}

void stopMotors() {
  Serial.println("Action: Stopping");
  digitalWrite(LEFT_MOTOR_IN1, LOW);
  digitalWrite(LEFT_MOTOR_IN2, LOW);
  digitalWrite(RIGHT_MOTOR_IN3, LOW);
  digitalWrite(RIGHT_MOTOR_IN4, LOW);
  
  ledcWrite(PWM_CHANNEL_LEFT, 0);
  ledcWrite(PWM_CHANNEL_RIGHT, 0);
}

void setupPWM() {
  ledcSetup(PWM_CHANNEL_LEFT, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_RIGHT, PWM_FREQUENCY, PWM_RESOLUTION);
  
  ledcAttachPin(LEFT_MOTOR_ENABLE, PWM_CHANNEL_LEFT);
  ledcAttachPin(RIGHT_MOTOR_ENABLE, PWM_CHANNEL_RIGHT);
  
  Serial.println("PWM configured for motor control");
}

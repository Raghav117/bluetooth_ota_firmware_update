# ESP32 BLE OTA Library for Arduino & PlatformIO

A comprehensive **Arduino library** for **ESP32 Bluetooth OTA firmware updates** using **BLE (Bluetooth Low Energy)**. Update ESP32 firmware wirelessly from mobile apps or BLE devices, ideal for **Arduino** and **PlatformIO** projects without USB or Wi-Fi.

[View the Wiki for detailed guides](https://github.com/Raghav117/bluetooth_ota_firmware_update/wiki) | [Explore Examples](https://github.com/Raghav117/bluetooth_ota_firmware_update/tree/main/examples)

## Table of Contents
- [Features](#features)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
  - [Constructors](#constructors)
  - [Initialization](#initialization)
  - [Callbacks](#callbacks)
  - [Status Methods](#status-methods)
  - [Configuration](#configuration)
  - [Control Methods](#control-methods)
- [OTA Protocol](#ota-protocol)
- [Examples](#examples)
- [Debugging](#debugging)
- [Changelog](#changelog)
- [Contributing](#contributing)
- [License](#license)
- [Further Reading](#further-reading)

## Features 🌟

- ✅ **Configurable UUIDs**: Use default or custom service/characteristic UUIDs for flexibility.
- ✅ **Progress Monitoring**: Real-time callbacks with percentage and byte count.
- ✅ **Status Reporting**: Detailed status updates (e.g., COMPLETED, ERROR) with error handling.
- ✅ **Command Interface**: Send custom commands (e.g., "LED_ON") alongside OTA.
- ✅ **Connection Management**: Automatic connection handling and re-advertising.
- ✅ **Error Recovery**: Robust mechanisms to handle OTA failures.
- ✅ **Multiple Examples**: Basic, advanced, custom UUIDs, and client apps for various platforms.

Perfect for **ESP32 OTA tutorials**, IoT projects, robotics, or consumer electronics.

## Installation 📦

Install via **Arduino Library Manager**:
1. Open Arduino IDE.
2. Go to **Tools > Manage Libraries**.
3. Search for **BLE OTA Update**.
4. Select the latest version and click **Install**.

For **PlatformIO**, add to your `platformio.ini`:
```ini
lib_deps = Raghav117/bluetooth_ota_firmware_update
```

Alternatively, manually install:
1. Download the [library folder](https://github.com/Raghav117/bluetooth_ota_firmware_update).
2. Place it in your Arduino `libraries` folder.
3. Restart Arduino IDE.

## Quick Start 🚀

Here’s a minimal **Bluetooth firmware update Arduino** example to set up OTA and control an LED:

```cpp
#include <BLEOtaUpdate.h>

BLEOtaUpdate bleOta;
const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Set callbacks
  bleOta.setOtaProgressCallback([](uint32_t received, uint32_t total, uint8_t percentage) {
    Serial.printf("Progress: %d%%\n", percentage);
    digitalWrite(LED_PIN, percentage % 2); // Blink LED during OTA
  });
  bleOta.setOtaStatusCallback([](OtaStatus status, const char* message) {
    Serial.printf("Status: %s\n", message);
    digitalWrite(LED_PIN, status == OTA_COMPLETED ? HIGH : LOW);
  });
  bleOta.setCommandCallback([](const String& command) {
    Serial.printf("Command: %s\n", command.c_str());
    if (command == "LED_ON") digitalWrite(LED_PIN, HIGH);
    else if (command == "LED_OFF") digitalWrite(LED_PIN, LOW);
  });

  // Start BLE OTA service
  bleOta.begin("MyESP32Device");
}

void loop() {
  delay(100); // No code needed; OTA is handled by the library
}
```

This example enables OTA updates, blinks an LED during progress, and handles commands like "LED_ON". See the [Wiki](https://github.com/Raghav117/bluetooth_ota_firmware_update/wiki) for detailed guides.

## API Reference 🛠️

### Constructors
```cpp
BLEOtaUpdate(); // Uses default UUIDs
BLEOtaUpdate(const char* serviceUUID, const char* otaCharUUID, const char* commandCharUUID, const char* statusCharUUID); // Custom UUIDs
```

### Initialization
```cpp
void begin(const char* deviceName); // Start with default UUIDs
void begin(const char* deviceName, const char* serviceUUID, const char* otaCharUUID, const char* commandCharUUID, const char* statusCharUUID); // Custom UUIDs
```

### Callbacks
```cpp
void setOtaProgressCallback(OtaProgressCallback callback); // Tracks progress (received, total, percentage)
void setOtaStatusCallback(OtaStatusCallback callback); // Reports status (e.g., COMPLETED, ERROR)
void setCommandCallback(CommandCallback callback); // Handles commands (e.g., "LED_ON")
void setConnectionCallback(ConnectionCallback callback); // Logs connection/disconnection
```

### Status Methods
```cpp
bool isConnected() const; // Check BLE connection
bool isUpdateInProgress() const; // Check if OTA is active
OtaStatus getOtaStatus() const; // Get current OTA status
uint32_t getUpdateProgress() const; // Bytes received
uint32_t getUpdateTotal() const; // Total bytes
uint8_t getUpdatePercentage() const; // Progress percentage
```

### Configuration
```cpp
void setServiceUUID(const char* uuid); // Set BLE service UUID
void setOtaCharacteristicUUID(const char* uuid); // Set OTA characteristic UUID
void setCommandCharacteristicUUID(const char* uuid); // Set command UUID
void setStatusCharacteristicUUID(const char* uuid); // Set status UUID
void setMaxPacketSize(size_t size); // Set max BLE packet size (e.g., 247)
void setUpdateBufferSize(size_t size); // Set buffer size for OTA
```

### Control Methods
```cpp
void stop(); // Stop BLE service
void restart(); // Restart BLE service
void abortUpdate(); // Cancel OTA
void sendStatus(const String& status); // Send status to client
void sendProgress(uint32_t received, uint32_t total); // Send progress update
```

### Callback Function Types
```cpp
typedef void (*OtaProgressCallback)(uint32_t received, uint32_t total, uint8_t percentage);
typedef void (*OtaStatusCallback)(OtaStatus status, const char* message);
typedef void (*CommandCallback)(const String& command);
typedef void (*ConnectionCallback)(bool connected);
```

### OTA Status Enum
```cpp
enum class OtaStatus {
  IDLE,        // No update in progress
  RECEIVING,   // Receiving firmware data
  COMPLETED,   // Update completed
  ERROR,       // Error during update
  ABORTED      // Update aborted
};
```

### Default UUIDs
- **Service UUID**: `12345678-1234-5678-9ABC-DEF012345678`
- **OTA Characteristic**: `87654321-4321-8765-CBA9-FEDCBA987654`
- **Command Characteristic**: `11111111-2222-3333-4444-555555555555`
- **Status Characteristic**: `AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE`

## OTA Protocol 📡

The library implements a robust OTA protocol:
1. **OPEN**: Client sends "OPEN" to start update.
2. **SIZE**: Client sends 4-byte firmware size.
3. **DATA**: Client sends firmware in chunks (based on MTU, typically 247 bytes).
4. **DONE**: Client sends "DONE" to finalize.
5. **ABORT**: Client can send "ABORT" to cancel.

See the [Wiki: OTA Client Guide](https://github.com/Raghav117/bluetooth_ota_firmware_update/wiki#writing-a-cross-platform-ota-client) for client implementation details.

## Examples 📚

| Example | Description |
|---------|-------------|
| [Basic Example](examples/BasicExample) | Minimal OTA setup for ESP32. |
| [Advanced Example](examples/AdvancedExample) | Progress tracking and error handling. |
| [Custom UUIDs](examples/CustomUUIDs) | Custom service/characteristic UUIDs. |
| [Robot OTA Example](examples/RobotOTAExample) | OTA in a robotics project. |
| [Flutter Client](examples/flutter) | Cross-platform app for OTA updates. |
| [Android (Kotlin) Client](examples/kotlin) | Android app for OTA updates. |
| [Python Client](examples/python/ota_client.py) | Python script for desktop OTA. |
| [Web Client](examples/web) | Web Bluetooth API for browser-based OTA. |

**Running an Example**:
1. Navigate to the example folder.
2. Open the `.ino` file or client code.
3. Ensure the ESP32 runs the OTA server code.
4. Follow setup instructions in the [Wiki](https://github.com/Raghav117/bluetooth_ota_firmware_update/wiki).

## Debugging 🔍

Enable Serial output for logs:
```cpp
Serial.begin(115200);
```

Check the [Wiki: Troubleshooting](https://github.com/Raghav117/bluetooth_ota_firmware_update/wiki#troubleshooting) for issues like MTU negotiation or UUID mismatches.

## Changelog 📜

- **v1.0.5 (2025-08-15)**: Added configurable UUIDs, progress/status callbacks, command interface, and examples.
- **v1.0.0 (2025-08-01)**: Initial release with core OTA functionality.

See [Releases](https://github.com/Raghav117/bluetooth_ota_firmware_update/releases) for updates.

## Contributing 🤝

Contributions are welcome! To contribute:
1. Open an issue on [GitHub](https://github.com/Raghav117/bluetooth_ota_firmware_update/issues).
2. Discuss on [X](https://x.com/search?q=ESP32%20BLE%20OTA) or [Arduino Forums](https://forum.arduino.cc/).
3. Submit a pull request with improvements.

Your feedback improves this **ESP32 OTA library**!

## License 📜

Provided as-is for educational and development purposes. See [LICENSE](LICENSE) for details.

## Further Reading 📖

- [Medium: ESP32 BLE OTA Tutorial](https://medium.com/@RaghavG1999/introducing-esp32-bluetooth-ota-firmware-update-library-89cec9b31d45)
- [Dev.to: Introducing ESP32 Bluetooth OTA](https://dev.to/raghav_garg_4b50a27930fe3/introducing-esp32-bluetooth-ota-firmware-update-library-64f)
- [Hashnode: Bluetooth OTA for ESP32](https://bleotaupdate.hashnode.dev/introducing-esp32-bluetooth-ota-firmware-update-library)
- [Arduino BLE Guide](https://www.arduino.cc/en/Reference/ArduinoBLE)
- [Espressif BLE Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/index.html)
- [PlatformIO Documentation](https://docs.platformio.org/en/latest/)

# BLE OTA Update Library

A comprehensive Arduino library for performing Over-The-Air (OTA) firmware updates over Bluetooth Low Energy (BLE) on ESP32 devices. This library provides a complete solution with configurable service UUIDs, characteristic UUIDs, progress callbacks, and command handling capabilities.

## Features

- **Configurable UUIDs**: Use default UUIDs or specify your own service and characteristic UUIDs
- **Progress Monitoring**: Real-time progress callbacks with percentage and byte count
- **Status Reporting**: Comprehensive status updates with error handling
- **Command Interface**: Built-in command handling for device control
- **Connection Management**: Automatic connection handling and re-advertising
- **Error Recovery**: Robust error handling and recovery mechanisms
- **Multiple Examples**: Basic, custom UUIDs, and advanced usage examples

## Installation

1. Download the library folder
2. Place it in your Arduino `libraries` folder
3. Restart the Arduino IDE
4. Include the library in your sketch: `#include <BLEOtaUpdate.h>`

## Quick Start

```cpp
#include <BLEOtaUpdate.h>

BLEOtaUpdate bleOta;

void setup() {
  Serial.begin(115200);
  
  // Set up callbacks
  bleOta.setOtaProgressCallback(onOtaProgress);
  bleOta.setOtaStatusCallback(onOtaStatus);
  bleOta.setCommandCallback(onCommand);
  
  // Start BLE OTA service
  bleOta.begin("MyESP32Device");
}

void loop() {
  delay(100);
}

void onOtaProgress(uint32_t received, uint32_t total, uint8_t percentage) {
  Serial.printf("Progress: %d%%\n", percentage);
}

void onOtaStatus(OtaStatus status, const char* message) {
  Serial.printf("Status: %s\n", message);
}

void onCommand(const String& command) {
  Serial.printf("Command: %s\n", command.c_str());
}
```

## API Reference

### Constructors

```cpp
BLEOtaUpdate();  // Use default UUIDs
BLEOtaUpdate(const char* serviceUUID, const char* otaCharUUID, const char* commandCharUUID, const char* statusCharUUID);
```

### Initialization

```cpp
void begin(const char* deviceName);
void begin(const char* deviceName, const char* serviceUUID, const char* otaCharUUID, const char* commandCharUUID, const char* statusCharUUID);
```

### Callbacks

```cpp
void setOtaProgressCallback(OtaProgressCallback callback);
void setOtaStatusCallback(OtaStatusCallback callback);
void setCommandCallback(CommandCallback callback);
void setConnectionCallback(ConnectionCallback callback);
```

### Status Methods

```cpp
bool isConnected() const;
bool isUpdateInProgress() const;
OtaStatus getOtaStatus() const;
uint32_t getUpdateProgress() const;
uint32_t getUpdateTotal() const;
uint8_t getUpdatePercentage() const;
```

### Configuration

```cpp
void setServiceUUID(const char* uuid);
void setOtaCharacteristicUUID(const char* uuid);
void setCommandCharacteristicUUID(const char* uuid);
void setStatusCharacteristicUUID(const char* uuid);
void setMaxPacketSize(size_t size);
void setUpdateBufferSize(size_t size);
```

### Control Methods

```cpp
void stop();
void restart();
void abortUpdate();
void sendStatus(const String& status);
void sendProgress(uint32_t received, uint32_t total);
```

## Callback Function Types

```cpp
typedef void (*OtaProgressCallback)(uint32_t received, uint32_t total, uint8_t percentage);
typedef void (*OtaStatusCallback)(OtaStatus status, const char* message);
typedef void (*CommandCallback)(const String& command);
typedef void (*ConnectionCallback)(bool connected);
```

## OTA Status Enum

```cpp
enum class OtaStatus {
  IDLE,        // No update in progress
  RECEIVING,   // Receiving firmware data
  COMPLETED,   // Update completed successfully
  ERROR,       // Error occurred during update
  ABORTED      // Update was aborted
};
```

## Default UUIDs

The library uses these default UUIDs if not specified:

- **Service UUID**: `12345678-1234-5678-9ABC-DEF012345678`
- **OTA Characteristic**: `87654321-4321-8765-CBA9-FEDCBA987654`
- **Command Characteristic**: `11111111-2222-3333-4444-555555555555`
- **Status Characteristic**: `AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE`

## OTA Protocol

The library implements a simple but robust OTA protocol:

1. **OPEN**: Client sends "OPEN" to start update
2. **SIZE**: Client sends 4-byte file size
3. **DATA**: Client sends firmware data in chunks
4. **DONE**: Client sends "DONE" to finalize update
5. **ABORT**: Client can send "ABORT" to cancel update

## Examples

This library includes ready-to-use examples for various platforms and use cases:

| Example | Description |
|---------|-------------|
| [**Basic Example**](examples/BasicExample) | Minimal setup for performing OTA firmware updates over Bluetooth. |
| [**Advanced Example**](examples/AdvancedExample) | Demonstrates advanced features like progress tracking and error handling. |
| [**Custom UUIDs**](examples/CustomUUIDs) | Shows how to use custom Service and Characteristic UUIDs. |
| [**Robot OTA Example**](examples/RobotOTAExample) | Demonstrates OTA updates in a robotics project. |
| [**Flutter Client**](examples/flutter) | Cross-platform Flutter app to trigger and monitor OTA updates. |
| [**Android (Kotlin) Client**](examples/kotlin) | Android app example using Kotlin for OTA firmware updates. |
| [**Python Client**](examples/python/ota_client.py) | Python script for initiating OTA updates from desktop or Raspberry Pi. |
| [**Web Client**](examples/web) | Web Bluetooth API example to perform OTA updates from a browser. |


---

### 🔹 Running an Example
1. Navigate to the example folder you want to try.
2. Open the example’s for setup instructions.
3. Ensure your ESP32 device is running the compatible OTA server code before starting.

### Debug Information

Enable Serial output to see detailed debug information:

```cpp
Serial.begin(115200);
```

## License

This library is provided as-is for educational and development purposes.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Version History

- **v1.0.4**: Release with OTA functionality
- Configurable UUIDs
- Progress and status callbacks
- Command interface
- Multiple examples
# bluetooth_ota_firmware_update

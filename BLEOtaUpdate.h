#ifndef BLE_OTA_UPDATE_H
#define BLE_OTA_UPDATE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Update.h>

// Default UUIDs - can be overridden
#define DEFAULT_SERVICE_UUID        "12345678-1234-5678-9ABC-DEF012345678"
#define DEFAULT_OTA_CHAR_UUID       "87654321-4321-8765-CBA9-FEDCBA987654"
#define DEFAULT_COMMAND_CHAR_UUID   "11111111-2222-3333-4444-555555555555"
#define DEFAULT_STATUS_CHAR_UUID    "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE"

// OTA Commands
#define OTA_CMD_OPEN    "OPEN"
#define OTA_CMD_DONE    "DONE"
#define OTA_CMD_ABORT   "ABORT"

// OTA Status
enum class OtaStatus {
  IDLE,
  RECEIVING,
  COMPLETED,
  ERROR,
  ABORTED
};

// Callback function types
typedef void (*OtaProgressCallback)(uint32_t received, uint32_t total, uint8_t percentage);
typedef void (*OtaStatusCallback)(OtaStatus status, const char* message);
typedef void (*CommandCallback)(const String& command);
typedef void (*ConnectionCallback)(bool connected);

class BLEOtaUpdate {
public:
  // Constructor with configurable parameters
  BLEOtaUpdate();
  BLEOtaUpdate(const char* serviceUUID, const char* otaCharUUID, const char* commandCharUUID, const char* statusCharUUID);
  
  // Initialize BLE OTA service
  void begin(const char* deviceName);
  void begin(const char* deviceName, const char* serviceUUID, const char* otaCharUUID, const char* commandCharUUID, const char* statusCharUUID);
  
  // Set callbacks
  void setOtaProgressCallback(OtaProgressCallback callback);
  void setOtaStatusCallback(OtaStatusCallback callback);
  void setCommandCallback(CommandCallback callback);
  void setConnectionCallback(ConnectionCallback callback);
  
  // Control methods
  void stop();
  void restart();
  void abortUpdate();
  
  // Status methods
  bool isConnected() const;
  bool isUpdateInProgress() const;
  OtaStatus getOtaStatus() const;
  uint32_t getUpdateProgress() const;
  uint32_t getUpdateTotal() const;
  uint8_t getUpdatePercentage() const;
  
  // Configuration methods
  void setServiceUUID(const char* uuid);
  void setOtaCharacteristicUUID(const char* uuid);
  void setCommandCharacteristicUUID(const char* uuid);
  void setStatusCharacteristicUUID(const char* uuid);
  void setMaxPacketSize(size_t size);
  void setUpdateBufferSize(size_t size);
  
  // Send status updates
  void sendStatus(const String& status);
  void sendProgress(uint32_t received, uint32_t total);
  
  // Loop method (call in main loop if needed)
  void loop();

private:
  // BLE components
  BLEServer* pServer;
  BLEService* pService;
  BLECharacteristic* pOtaCharacteristic;
  BLECharacteristic* pCommandCharacteristic;
  BLECharacteristic* pStatusCharacteristic;
  
  // UUIDs
  String serviceUUID;
  String otaCharUUID;
  String commandCharUUID;
  String statusCharUUID;
  
  // OTA state
  bool otaInProgress;
  uint32_t otaFileSize;
  uint32_t otaReceived;
  OtaStatus otaStatus;
  bool clientConnected;
  
  // Configuration
  size_t maxPacketSize;
  size_t updateBufferSize;
  
  // Callbacks
  OtaProgressCallback progressCallback;
  OtaStatusCallback statusCallback;
  CommandCallback commandCallback;
  ConnectionCallback connectionCallback;
  
  // Internal methods
  void initializeService();
  void handleOtaWrite(BLECharacteristic* pCharacteristic);
  void handleCommandWrite(BLECharacteristic* pCharacteristic);
  void onClientConnect();
  void onClientDisconnect();
  void updateProgress();
  void setOtaStatus(OtaStatus status, const char* message = nullptr);
  
  // BLE callback classes
  class ServerCallbacks : public BLEServerCallbacks {
  public:
    ServerCallbacks() {}
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
  };

  class OtaCharacteristicCallbacks : public BLECharacteristicCallbacks {
  public:
    OtaCharacteristicCallbacks() {}
    void onWrite(BLECharacteristic* pCharacteristic) override;
  };

  class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks {
  public:
    CommandCharacteristicCallbacks() {}
    void onWrite(BLECharacteristic* pCharacteristic) override;
  };
  
  // Static instance for callbacks
  static BLEOtaUpdate* instance;
};

#endif // BLE_OTA_UPDATE_H

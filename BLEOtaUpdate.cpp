#include "BLEOtaUpdate.h"

// Static instance
BLEOtaUpdate* BLEOtaUpdate::instance = nullptr;

// Constructor implementations
BLEOtaUpdate::BLEOtaUpdate() 
  : BLEOtaUpdate(DEFAULT_SERVICE_UUID, DEFAULT_OTA_CHAR_UUID, DEFAULT_COMMAND_CHAR_UUID, DEFAULT_STATUS_CHAR_UUID) {
}

BLEOtaUpdate::BLEOtaUpdate(const char* serviceUUID, const char* otaCharUUID, const char* commandCharUUID, const char* statusCharUUID) {
  this->serviceUUID = String(serviceUUID);
  this->otaCharUUID = String(otaCharUUID);
  this->commandCharUUID = String(commandCharUUID);
  this->statusCharUUID = String(statusCharUUID);
  
  // Initialize state
  otaInProgress = false;
  otaFileSize = 0;
  otaReceived = 0;
  otaStatus = OtaStatus::IDLE;
  clientConnected = false;
  
  // Default configuration
  maxPacketSize = 512;
  updateBufferSize = 4096;
  
  // Initialize callbacks
  progressCallback = nullptr;
  statusCallback = nullptr;
  commandCallback = nullptr;
  connectionCallback = nullptr;
  
  // Initialize BLE components
  pServer = nullptr;
  pService = nullptr;
  pOtaCharacteristic = nullptr;
  pCommandCharacteristic = nullptr;
  pStatusCharacteristic = nullptr;
}

void BLEOtaUpdate::begin(const char* deviceName) {
  begin(deviceName, serviceUUID.c_str(), otaCharUUID.c_str(), commandCharUUID.c_str(), statusCharUUID.c_str());
}

void BLEOtaUpdate::begin(const char* deviceName, const char* serviceUUID, const char* otaCharUUID, const char* commandCharUUID, const char* statusCharUUID) {
  // Set instance for callbacks
  instance = this;
  
  // Update UUIDs if provided
  if (serviceUUID) this->serviceUUID = String(serviceUUID);
  if (otaCharUUID) this->otaCharUUID = String(otaCharUUID);
  if (commandCharUUID) this->commandCharUUID = String(commandCharUUID);
  if (statusCharUUID) this->statusCharUUID = String(statusCharUUID);
  
  // Initialize BLE device
  BLEDevice::init(deviceName);
  
  // Create server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  // Initialize service
  initializeService();
  
  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(this->serviceUUID.c_str());
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  setOtaStatus(OtaStatus::IDLE, "BLE OTA Service Ready");
  Serial.println("[BLE OTA] Service started and advertising");
}

void BLEOtaUpdate::initializeService() {
  // Create service
  pService = pServer->createService(serviceUUID.c_str());
  
  // Create OTA characteristic
  pOtaCharacteristic = pService->createCharacteristic(
    otaCharUUID.c_str(),
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pOtaCharacteristic->setCallbacks(new OtaCharacteristicCallbacks());
  pOtaCharacteristic->addDescriptor(new BLE2902());
  
  // Create command characteristic
  pCommandCharacteristic = pService->createCharacteristic(
    commandCharUUID.c_str(),
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR
  );
  pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
  
  // Create status characteristic
  pStatusCharacteristic = pService->createCharacteristic(
    statusCharUUID.c_str(),
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pStatusCharacteristic->addDescriptor(new BLE2902());
  
  // Start service
  pService->start();
}

// Callback setters
void BLEOtaUpdate::setOtaProgressCallback(OtaProgressCallback callback) {
  progressCallback = callback;
}

void BLEOtaUpdate::setOtaStatusCallback(OtaStatusCallback callback) {
  statusCallback = callback;
}

void BLEOtaUpdate::setCommandCallback(CommandCallback callback) {
  commandCallback = callback;
}

void BLEOtaUpdate::setConnectionCallback(ConnectionCallback callback) {
  connectionCallback = callback;
}

// Control methods
void BLEOtaUpdate::stop() {
  if (pServer) {
    BLEDevice::stopAdvertising();
  }
  setOtaStatus(OtaStatus::IDLE, "Service stopped");
}

void BLEOtaUpdate::restart() {
  if (pServer) {
    BLEDevice::startAdvertising();
    setOtaStatus(OtaStatus::IDLE, "Service restarted");
  }
}

void BLEOtaUpdate::abortUpdate() {
  if (otaInProgress) {
    Update.end(false);
    otaInProgress = false;
    otaFileSize = 0;
    otaReceived = 0;
    setOtaStatus(OtaStatus::ABORTED, "Update aborted by user");
  }
}

// Status methods
bool BLEOtaUpdate::isConnected() const {
  return clientConnected;
}

bool BLEOtaUpdate::isUpdateInProgress() const {
  return otaInProgress;
}

OtaStatus BLEOtaUpdate::getOtaStatus() const {
  return otaStatus;
}

uint32_t BLEOtaUpdate::getUpdateProgress() const {
  return otaReceived;
}

uint32_t BLEOtaUpdate::getUpdateTotal() const {
  return otaFileSize;
}

uint8_t BLEOtaUpdate::getUpdatePercentage() const {
  if (otaFileSize == 0) return 0;
  return (otaReceived * 100) / otaFileSize;
}

// Configuration methods
void BLEOtaUpdate::setServiceUUID(const char* uuid) {
  serviceUUID = String(uuid);
}

void BLEOtaUpdate::setOtaCharacteristicUUID(const char* uuid) {
  otaCharUUID = String(uuid);
}

void BLEOtaUpdate::setCommandCharacteristicUUID(const char* uuid) {
  commandCharUUID = String(uuid);
}

void BLEOtaUpdate::setStatusCharacteristicUUID(const char* uuid) {
  statusCharUUID = String(uuid);
}

void BLEOtaUpdate::setMaxPacketSize(size_t size) {
  maxPacketSize = size;
}

void BLEOtaUpdate::setUpdateBufferSize(size_t size) {
  updateBufferSize = size;
}

// Send status updates
void BLEOtaUpdate::sendStatus(const String& status) {
  if (pStatusCharacteristic && clientConnected) {
    pStatusCharacteristic->setValue(status.c_str());
    pStatusCharacteristic->notify();
  }
}

void BLEOtaUpdate::sendProgress(uint32_t received, uint32_t total) {
  if (pStatusCharacteristic && clientConnected) {
    String progress = "PROGRESS:" + String(received) + "/" + String(total);
    pStatusCharacteristic->setValue(progress.c_str());
    pStatusCharacteristic->notify();
  }
}

// Loop method
void BLEOtaUpdate::loop() {
  // This method can be used for periodic tasks if needed
  // Currently empty as all work is done in callbacks
}

// Internal methods
void BLEOtaUpdate::handleOtaWrite(BLECharacteristic* pCharacteristic) {
  std::string value = pCharacteristic->getValue().c_str();
  const uint8_t* data = (const uint8_t*)value.c_str();
  size_t length = value.length();

  if (length == 0) return;

  // Handle OTA commands
  if (!otaInProgress && length == 4) {
    if (memcmp(data, OTA_CMD_OPEN, 4) == 0) {
      Serial.println("[OTA] Update started");
      otaInProgress = true;
      otaFileSize = 0;
      otaReceived = 0;
      setOtaStatus(OtaStatus::RECEIVING, "Update started");
      return;
    }
  }

  if (otaInProgress) {
    // Handle file size (first 4 bytes after OPEN)
    if (otaFileSize == 0 && length == 4) {
      memcpy(&otaFileSize, data, 4);
      Serial.printf("[OTA] Update size: %u bytes\n", otaFileSize);
      
      if (!Update.begin(otaFileSize)) {
        Serial.println("[OTA] ERROR: Not enough space");
        setOtaStatus(OtaStatus::ERROR, "Not enough space");
        otaInProgress = false;
        return;
      }
      setOtaStatus(OtaStatus::RECEIVING, "Receiving firmware");
      return;
    }

    // Handle DONE command
    if (length == 4 && memcmp(data, OTA_CMD_DONE, 4) == 0) {
      Serial.println("[OTA] Finalizing update...");
      
      if (otaReceived != otaFileSize) {
        Serial.printf("[OTA] ERROR: Size mismatch! (%u/%u)\n", otaReceived, otaFileSize);
        setOtaStatus(OtaStatus::ERROR, "Size mismatch");
        Update.end(false);
      } else if (Update.end(true)) {
        Serial.println("[OTA] Success. Rebooting...");
        setOtaStatus(OtaStatus::COMPLETED, "Update completed successfully");
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("[OTA] Finalize failed");
        setOtaStatus(OtaStatus::ERROR, "Update finalization failed");
        Update.printError(Serial);
      }
      otaInProgress = false;
      return;
    }

    // Handle ABORT command
    if (length == 5 && memcmp(data, OTA_CMD_ABORT, 5) == 0) {
      Serial.println("[OTA] Update aborted by client");
      abortUpdate();
      return;
    }

    // Handle firmware data
    if (otaReceived < otaFileSize) {
      size_t written = Update.write((uint8_t*)data, length);
      delay(1);
      if (written > 0) {
        otaReceived += written;
        updateProgress();
      } else {
        Serial.println("[OTA] ERROR: Write failed");
        setOtaStatus(OtaStatus::ERROR, "Write failed");
        otaInProgress = false;
      }
    }
  }
}

void BLEOtaUpdate::handleCommandWrite(BLECharacteristic* pCharacteristic) {
  std::string value = pCharacteristic->getValue().c_str();
  if (commandCallback && value.length() > 0) {
    String command = String(value.c_str());
    commandCallback(command);
  }
}

void BLEOtaUpdate::onClientConnect() {
  clientConnected = true;
  Serial.println("[BLE] Client connected");
  if (connectionCallback) {
    connectionCallback(true);
  }
  sendStatus("Connected");
}

void BLEOtaUpdate::onClientDisconnect() {
  clientConnected = false;
  if (otaInProgress) {
    Serial.println("[OTA] ERROR: Client disconnected during update");
    abortUpdate();
  }
  Serial.println("[BLE] Client disconnected. Re-advertising...");
  if (connectionCallback) {
    connectionCallback(false);
  }
  BLEDevice::startAdvertising();
}

void BLEOtaUpdate::updateProgress() {
  uint8_t percentage = getUpdatePercentage();
  Serial.printf("[OTA] Progress: %d%% (%u/%u)\r", percentage, otaReceived, otaFileSize);
  
  if (progressCallback) {
    progressCallback(otaReceived, otaFileSize, percentage);
  }
  
  sendProgress(otaReceived, otaFileSize);
}

void BLEOtaUpdate::setOtaStatus(OtaStatus status, const char* message) {
  otaStatus = status;
  if (statusCallback) {
    statusCallback(status, message);
  }
  if (message) {
    Serial.printf("[OTA Status] %s\n", message);
  }
}

// BLE Callback Classes Implementation
void BLEOtaUpdate::ServerCallbacks::onConnect(BLEServer* pServer) {
  if (instance) {
    instance->onClientConnect();
  }
}

void BLEOtaUpdate::ServerCallbacks::onDisconnect(BLEServer* pServer) {
  if (instance) {
    instance->onClientDisconnect();
  }
}

void BLEOtaUpdate::OtaCharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
  if (instance) {
    instance->handleOtaWrite(pCharacteristic);
  }
}

void BLEOtaUpdate::CommandCharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
  if (instance) {
    instance->handleCommandWrite(pCharacteristic);
  }
}
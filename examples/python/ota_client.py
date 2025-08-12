"""
ESP32 BLE OTA Client (Python)
Requires: bleak >= 0.20
Install: pip install bleak
"""

import asyncio
from bleak import BleakClient, BleakScanner

# Replace with your ESP32's BLE name and UUIDs
DEVICE_NAME = "ESP32_OTA"
OTA_SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
OTA_CHARACTERISTIC_UUID = "abcdef01-1234-5678-1234-56789abcdef0"

# Path to the firmware binary
FIRMWARE_FILE = "firmware.bin"

async def ota_update():
    print(f"🔍 Scanning for {DEVICE_NAME}...")
    devices = await BleakScanner.discover()
    target_device = next((d for d in devices if DEVICE_NAME in d.name), None)

    if not target_device:
        print(f"❌ Device '{DEVICE_NAME}' not found. Make sure it's in OTA mode.")
        return

    print(f"✅ Found {target_device.name} [{target_device.address}]")
    
    async with BleakClient(target_device.address) as client:
        print("🔗 Connecting...")
        if not await client.is_connected():
            print("❌ Failed to connect")
            return
        print("✅ Connected")

        # Read firmware
        try:
            with open(FIRMWARE_FILE, "rb") as f:
                firmware_data = f.read()
        except FileNotFoundError:
            print(f"❌ Firmware file '{FIRMWARE_FILE}' not found.")
            return

        print(f"📦 Sending firmware ({len(firmware_data)} bytes)...")
        chunk_size = 512  # Adjust based on your ESP32 config
        for i in range(0, len(firmware_data), chunk_size):
            chunk = firmware_data[i:i+chunk_size]
            await client.write_gatt_char(OTA_CHARACTERISTIC_UUID, chunk)
            progress = (i + chunk_size) / len(firmware_data) * 100
            print(f"   {progress:.1f}% complete", end="\r")
        
        print("\n✅ OTA update complete")

if __name__ == "__main__":
    asyncio.run(ota_update())

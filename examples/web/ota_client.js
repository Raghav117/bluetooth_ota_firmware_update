// ===== Configure your UUIDs (defaults from the library) =====
const SERVICE_UUID = '12345678-1234-5678-9ABC-DEF012345678';
const OTA_CHAR_UUID = '87654321-4321-8765-CBA9-FEDCBA987654';

// 1) Ask the user to choose a .bin firmware file
async function pickFirmware() {
    const [handle] = await window.showOpenFilePicker({
        types: [{ description: 'Firmware', accept: { 'application/octet-stream': ['.bin'] } }]
    });
    const file = await handle.getFile();
    return await file.arrayBuffer();
}

// 2) Connect and discover the OTA characteristic (user gesture required)
async function connectAndDiscover() {
    const device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [SERVICE_UUID] }]
    });
    const server = await device.gatt.connect();
    const service = await server.getPrimaryService(SERVICE_UUID);
    const otaChar = await service.getCharacteristic(OTA_CHAR_UUID);
    return { device, server, otaChar };
}

// 3) Perform the OTA transfer: OPEN → SIZE → DATA → DONE
async function performOtaTransfer(otaChar, firmwareArrayBuffer) {
    // OPEN (with response)
    await otaChar.writeValue(new TextEncoder().encode("OPEN"));

    // SIZE (4-byte little-endian, with response)
    const sizeBuf = new ArrayBuffer(4);
    new DataView(sizeBuf).setUint32(0, firmwareArrayBuffer.byteLength, true);
    await otaChar.writeValue(new Uint8Array(sizeBuf));

    // DATA (chunked writes + pacing with 30ms delay)
    const fw = new Uint8Array(firmwareArrayBuffer);
    const chunk = 128;
    for (let i = 0; i < fw.length; i += chunk) {
        await otaChar.writeValue(fw.slice(i, Math.min(i + chunk, fw.length)));

        // 30 milliseconds delay after each chunk to avoid packet loss
        await new Promise(r => setTimeout(r, 30));
    }

    // DONE (with response)
    await otaChar.writeValue(new TextEncoder().encode("DONE"));
}

// 4) Disconnect safely
async function disconnectSafely(server) {
    try { server.disconnect(); } catch (e) { }
}

// ===== Main Orchestrator =====
// Wire this to a "Start Update" button click handler on your page
async function main() {
    // Connect + discover
    const { server, otaChar } = await connectAndDiscover();

    try {
        // Pick file
        const fwBuf = await pickFirmware();

        // OTA transfer
        await performOtaTransfer(otaChar, fwBuf);
    } finally {
        // Cleanup (device reboots after DONE)
        await disconnectSafely(server);
    }
}

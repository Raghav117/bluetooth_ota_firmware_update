import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import android.os.ParcelUuid
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.delay
import kotlinx.coroutines.suspendCancellableCoroutine
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.*
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

// ===== UUIDs =====
val SERVICE_UUID: UUID = UUID.fromString("12345678-1234-5678-9ABC-DEF012345678")
val OTA_CHAR_UUID: UUID = UUID.fromString("87654321-4321-8765-CBA9-FEDCBA987654")

// ---- 1) Scan for device ----
suspend fun scanForDeviceOnce(scanner: BluetoothLeScanner, timeoutMs: Long = 8000): BluetoothDevice? =
    suspendCancellableCoroutine { cont ->
        val filter = ScanFilter.Builder().setServiceUuid(ParcelUuid(SERVICE_UUID)).build()
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()

        val cb = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                scanner.stopScan(this)
                cont.resume(result.device)
            }

            override fun onScanFailed(errorCode: Int) {
                cont.resumeWithException(Exception("Scan failed: $errorCode"))
            }
        }

        scanner.startScan(listOf(filter), settings, cb)

        cont.invokeOnCancellation { scanner.stopScan(cb) }

        // Timeout
        cont.context.job.invokeOnCompletion {
            scanner.stopScan(cb)
        }
    }

// ---- 2) Connect & discover services ----
suspend fun connectAndDiscover(context: Context, device: BluetoothDevice): BluetoothGatt =
    suspendCancellableCoroutine { cont ->
        val callback = object : BluetoothGattCallback() {
            override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
                if (status != BluetoothGatt.GATT_SUCCESS) {
                    cont.resumeWithException(Exception("Connection failed: $status"))
                    gatt.close()
                    return
                }
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    gatt.discoverServices()
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    cont.resumeWithException(Exception("Disconnected"))
                    gatt.close()
                }
            }

            override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    // Request MTU (optional, Android only)
                    gatt.requestMtu(247)
                } else {
                    cont.resumeWithException(Exception("Service discovery failed"))
                    gatt.close()
                }
            }

            override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
                cont.resume(gatt)
            }
        }

        val gatt = device.connectGatt(context, false, callback)
        cont.invokeOnCancellation { gatt.close() }
    }

// ---- 3) Find OTA characteristic ----
fun findOtaCharacteristic(gatt: BluetoothGatt): BluetoothGattCharacteristic {
    val svc = gatt.getService(SERVICE_UUID) ?: error("Service not found")
    return svc.getCharacteristic(OTA_CHAR_UUID) ?: error("OTA characteristic not found")
}

// ---- 4) Write helper (suspend until confirmed) ----
suspend fun write(
    gatt: BluetoothGatt,
    ch: BluetoothGattCharacteristic,
    data: ByteArray,
    withResponse: Boolean
) {
    val result = CompletableDeferred<Boolean>()
    val callback = object : BluetoothGattCallback() {
        override fun onCharacteristicWrite(g: BluetoothGatt, c: BluetoothGattCharacteristic, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                result.complete(true)
            } else {
                result.completeExceptionally(Exception("Write failed: $status"))
            }
        }
    }

    ch.writeType = if (withResponse)
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
    else
        BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE

    ch.value = data

    if (!gatt.writeCharacteristic(ch)) {
        throw Exception("writeCharacteristic failed to initiate")
    }

    result.await()

    // Add pacing delay after each write to prevent ESP32 OTA packet loss
    delay(30) // 30 ms is safe; adjust if stable at lower values
}

// ---- 5) OTA transfer ----
suspend fun performOtaTransfer(gatt: BluetoothGatt, ch: BluetoothGattCharacteristic, firmware: ByteArray) {
    val payload = (gatt.mtu - 3).coerceAtLeast(20)
    println("📦 Sending ${firmware.size} bytes in chunks of $payload")

    // OPEN
    write(gatt, ch, "OPEN".toByteArray(Charsets.UTF_8), withResponse = true)

    // SIZE
    val sizeBytes = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(firmware.size).array()
    write(gatt, ch, sizeBytes, withResponse = true)

    // DATA
    var off = 0
    while (off < firmware.size) {
        val end = (off + payload).coerceAtMost(firmware.size)
        write(gatt, ch, firmware.copyOfRange(off, end), withResponse = false)
        off = end
         // Add pacing delay after each write to prevent ESP32 OTA packet loss
    delay(30) // 30 ms is safe; adjust if stable at lower values
    }

    // DONE
    write(gatt, ch, "DONE".toByteArray(Charsets.UTF_8), withResponse = true)
    println("✅ OTA update completed")
}

// ---- 6) Cleanup ----
fun disconnectSafely(gatt: BluetoothGatt) {
    try { gatt.disconnect() } catch (_: Exception) {}
    try { gatt.close() } catch (_: Exception) {}
}

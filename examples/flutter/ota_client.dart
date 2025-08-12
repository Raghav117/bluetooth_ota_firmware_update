import 'dart:convert';
import 'dart:typed_data';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:file_picker/file_picker.dart';

// ===== UUIDs =====
final serviceUuid = Guid("12345678-1234-5678-9ABC-DEF012345678");
final otaCharUuid = Guid("87654321-4321-8765-CBA9-FEDCBA987654");

// Scan for device exposing the OTA service
Future<BluetoothDevice?> scanForDevice() async {
  print("🔍 Scanning for OTA-enabled device...");
  await FlutterBluePlus.startScan(
    withServices: [serviceUuid],
    timeout: const Duration(seconds: 8),
  );

  final results = await FlutterBluePlus.scanResults.firstWhere(
    (list) => list.isNotEmpty,
    orElse: () => [],
  );

  await FlutterBluePlus.stopScan();
  return results.isNotEmpty ? results.first.device : null;
}

// Connect & discover OTA characteristic
Future<(BluetoothDevice device, BluetoothCharacteristic otaChar)?>
connectAndDiscoverOta(BluetoothDevice d) async {
  print("🔗 Connecting to ${d.platformName}...");
  await d.connect(timeout: const Duration(seconds: 15));

  try {
    await d.requestMtu(247);
  } catch (e) {
    print("⚠️ MTU request failed: $e");
  }

  final services = await d.discoverServices();
  final s = services.firstWhere(
    (x) => x.uuid == serviceUuid,
    orElse: () => throw Exception("Service not found"),
  );
  final c = s.characteristics.firstWhere(
    (x) => x.uuid == otaCharUuid,
    orElse: () => throw Exception("OTA characteristic not found"),
  );

  return (d, c);
}

// Let user pick firmware file
Future<Uint8List?> pickFirmwareBin() async {
  final r = await FilePicker.platform.pickFiles(
    type: FileType.custom,
    allowedExtensions: ['bin'],
  );
  return r?.files.first.bytes;
}

// OTA transfer sequence
Future<void> performOtaTransfer(
  BluetoothDevice d,
  BluetoothCharacteristic otaChar,
  Uint8List fw,
) async {
  final chunkSize = (d.mtuNow > 0 ? d.mtuNow : 23) - 3;
  print("📦 Sending firmware (${fw.length} bytes) in chunks of $chunkSize...");

  // OPEN
  await otaChar.write(utf8.encode("OPEN"), withoutResponse: false);

  // SIZE
  final size = ByteData(4)..setUint32(0, fw.length, Endian.little);
  await otaChar.write(size.buffer.asUint8List(), withoutResponse: false);

  // DATA
  for (var i = 0; i < fw.length; i += chunkSize) {
    final end = (i + chunkSize > fw.length) ? fw.length : i + chunkSize;
    await otaChar.write(fw.sublist(i, end), withoutResponse: true);
    await Future.delayed(const Duration(milliseconds: 2));
  }

  // DONE
  await otaChar.write(utf8.encode("DONE"), withoutResponse: false);
  print("✅ OTA update completed");
}

// Disconnect
Future<void> disconnectSafely(BluetoothDevice d) async {
  try {
    await d.disconnect();
  } catch (_) {}
}

// Orchestrator
Future<void> runOtaFlow() async {
  final dev = await scanForDevice();
  if (dev == null) throw Exception("No OTA device found");

  final result = await connectAndDiscoverOta(dev);
  if (result == null) throw Exception("OTA characteristic not found");
  final (device, otaChar) = result;

  final fw = await pickFirmwareBin();
  if (fw == null) throw Exception("No firmware selected");

  try {
    await performOtaTransfer(device, otaChar, fw);
  } finally {
    await disconnectSafely(device);
  }
}

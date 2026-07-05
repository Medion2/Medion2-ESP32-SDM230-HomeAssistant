# Installation

## 1. Requirements

- Visual Studio Code
- PlatformIO extension
- ESP32-S3 DevKitC-1
- MAX485 module
- Eastron SDM230 Modbus meter

## 2. Configure secrets

Copy the example configuration:

```bash
cp include/config.example.h include/config.h
```

Edit `include/config.h` and enter your own WiFi and MQTT settings.

`include/config.h` is ignored by Git and must not be committed.

## 3. First upload by USB

Connect the ESP32-S3 by USB and run:

```bash
pio run -t upload
```

Open the serial monitor:

```bash
pio device monitor
```

## 4. OTA upload

After the first upload, OTA can be enabled in `platformio.ini`:

```ini
upload_protocol = espota
upload_port = esp32-sdm230.local
```

Then upload again:

```bash
pio run -t upload
```

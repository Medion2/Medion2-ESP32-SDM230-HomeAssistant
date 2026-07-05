# ESP32-SDM230-HomeAssistant

ESP32-S3 + MAX485 firmware for reading an Eastron SDM230 Modbus energy meter and publishing the values to Home Assistant via MQTT Discovery.

## Features

- ESP32-S3 DevKitC-1 support
- MAX485 RS485 interface
- Eastron SDM230 Modbus RTU
- Web dashboard with live values
- JSON API
- CSV export
- OTA update support
- MQTT publishing
- Home Assistant MQTT Discovery
- Restart button in Home Assistant
- WiFi RSSI, uptime and free heap sensors
- No passwords or private IP addresses stored in the repository

## Hardware

- ESP32-S3 DevKitC-1
- MAX485 TTL to RS485 module
- Eastron SDM230 Modbus energy meter

## Wiring

### ESP32-S3 to MAX485

| ESP32-S3 | MAX485 |
|---|---|
| GPIO17 | DI |
| GPIO18 | RO |
| GPIO4 | DE |
| GPIO4 | RE |
| 5V | VCC |
| GND | GND |

### SDM230 to MAX485

| SDM230 | MAX485 |
|---|---|
| Terminal 5 A+ | A |
| Terminal 6 B- | B |
| Terminal 7 GND | GND |

If communication does not work, swap A and B.

## PlatformIO setup

1. Install PlatformIO.
2. Clone this repository.
3. Copy `include/config.example.h` to `include/config.h`.
4. Enter your WiFi and MQTT settings in `include/config.h`.
5. Build and upload.

```bash
pio run
pio run -t upload
```

## OTA update

After the first USB flash, OTA can be used with the hostname:

```text
esp32-sdm230.local
```

You can enable OTA upload in `platformio.ini` by uncommenting:

```ini
upload_protocol = espota
upload_port = esp32-sdm230.local
```

## Web endpoints

| Endpoint | Description |
|---|---|
| `/` | Web dashboard |
| `/json` | JSON API |
| `/csv` | CSV export |
| `/discovery` | Send Home Assistant MQTT Discovery again |
| `/restart` | Restart the ESP32 |

## SDM230 default settings

The firmware is prepared for:

- Modbus ID: `1`
- Baudrate: `2400`
- Serial format: `8N1`

Adjust these in `include/config.h` if needed.

## License

MIT License

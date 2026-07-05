# Wiring

## ESP32-S3 to MAX485

| ESP32-S3 | MAX485 |
|---|---|
| GPIO17 | DI |
| GPIO18 | RO |
| GPIO4 | DE |
| GPIO4 | RE |
| 5V | VCC |
| GND | GND |

DE and RE must be connected together and connected to GPIO4.

## SDM230 to MAX485

| SDM230 | MAX485 |
|---|---|
| Terminal 5 A+ | A |
| Terminal 6 B- | B |
| Terminal 7 GND | GND |

If the web dashboard shows timeout or no values, swap A and B.

## Default Modbus settings

- Modbus ID: 1
- Baudrate: 2400
- Format: 8N1

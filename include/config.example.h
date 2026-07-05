#pragma once

// Copy this file to include/config.h and enter your own values.
// Do not commit include/config.h to GitHub.

// WiFi
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Optional static IP. Set to 0 for DHCP, 1 for static IP.
#define USE_STATIC_IP 0
#define STATIC_IP_ADDR 192,168,1,166
#define GATEWAY_IP_ADDR 192,168,1,1
#define SUBNET_MASK_ADDR 255,255,255,0
#define DNS_IP_ADDR 192,168,1,1

// MQTT / Home Assistant
#define MQTT_SERVER "YOUR_HOME_ASSISTANT_OR_MQTT_IP"
#define MQTT_PORT 1883
#define MQTT_USER "YOUR_MQTT_USER"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"

// Device
#define DEVICE_NAME "esp32_sdm230"
#define DEVICE_PRETTY_NAME "ESP32 SDM230"

// ESP32-S3 + MAX485 pins
#define PIN_RS485_RX 18
#define PIN_RS485_TX 17
#define PIN_RS485_DE_RE 4

// SDM230 Modbus settings
#define SDM230_MODBUS_ID 1
#define SDM230_BAUDRATE 2400

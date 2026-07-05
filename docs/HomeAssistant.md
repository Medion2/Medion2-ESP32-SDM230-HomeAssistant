# Home Assistant MQTT

## Mosquitto broker

Install and start the Mosquitto broker add-on in Home Assistant.

Create a separate MQTT user and enter the credentials in `include/config.h`.

## MQTT Discovery

The firmware publishes Home Assistant MQTT Discovery messages automatically after MQTT connection.

To resend discovery manually, open:

```text
http://<esp-ip>/discovery
```

## MQTT topics

| Topic | Description |
|---|---|
| `esp32_sdm230/state` | JSON sensor values |
| `esp32_sdm230/availability` | online/offline state |
| `esp32_sdm230/restart` | restart command |
| `homeassistant/.../config` | discovery topics |

## Test topic

In Home Assistant MQTT integration, listen to:

```text
esp32_sdm230/state
```

You should receive JSON values from the ESP32.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>
#include "config.h"

static const char* SW_VERSION = "3.1.0";

WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(espClient);
ModbusMaster node;

float voltage = NAN;
float current = NAN;
float power = NAN;
float apparent_power = NAN;
float reactive_power = NAN;
float power_factor = NAN;
float frequency = NAN;
float import_kwh = NAN;
float export_kwh = NAN;
float min_power = NAN;
float max_power = NAN;
float boot_import = NAN;
float boot_export = NAN;

uint8_t last_result = 255;
String last_status = "No request yet";
unsigned long last_read_ms = 0;
unsigned long last_mqtt_ms = 0;

static String topic(const char* suffix) {
  return String(DEVICE_NAME) + "/" + suffix;
}

void preTransmission() {
  digitalWrite(PIN_RS485_DE_RE, HIGH);
  delayMicroseconds(300);
}

void postTransmission() {
  delayMicroseconds(300);
  digitalWrite(PIN_RS485_DE_RE, LOW);
}

String modbusStatusText(uint8_t result) {
  if (result == 0) return "OK";
  if (result == 226) return "Timeout / no answer";
  if (result == 224) return "Invalid response";
  if (result == 225) return "CRC error";
  if (result == 2) return "Illegal data address";
  if (result == 3) return "Illegal data value";
  return "Error code: " + String(result);
}

float readFloatRegister(uint16_t reg) {
  uint8_t result = node.readInputRegisters(reg, 2);
  last_result = result;
  last_status = modbusStatusText(result);

  if (result == node.ku8MBSuccess) {
    uint16_t hi = node.getResponseBuffer(0);
    uint16_t lo = node.getResponseBuffer(1);
    uint32_t raw = ((uint32_t)hi << 16) | lo;
    float value;
    memcpy(&value, &raw, sizeof(value));
    return value;
  }
  return NAN;
}

void readSDM230() {
  voltage        = readFloatRegister(0x0000);
  current        = readFloatRegister(0x0006);
  power          = readFloatRegister(0x000C);
  apparent_power = readFloatRegister(0x0012);
  reactive_power = readFloatRegister(0x0018);
  power_factor   = readFloatRegister(0x001E);
  frequency      = readFloatRegister(0x0046);
  import_kwh     = readFloatRegister(0x0048);
  export_kwh     = readFloatRegister(0x004A);

  if (!isnan(power)) {
    if (isnan(min_power) || power < min_power) min_power = power;
    if (isnan(max_power) || power > max_power) max_power = power;
  }
  if (!isnan(import_kwh) && isnan(boot_import)) boot_import = import_kwh;
  if (!isnan(export_kwh) && isnan(boot_export)) boot_export = export_kwh;
}

String num(float value, int decimals) {
  if (isnan(value)) return "null";
  return String(value, decimals);
}

String jsonState() {
  String json = "{";
  json += "\"status\":\"" + last_status + "\",";
  json += "\"voltage\":" + num(voltage, 1) + ",";
  json += "\"current\":" + num(current, 3) + ",";
  json += "\"power\":" + num(power, 1) + ",";
  json += "\"apparent_power\":" + num(apparent_power, 1) + ",";
  json += "\"reactive_power\":" + num(reactive_power, 1) + ",";
  json += "\"power_factor\":" + num(power_factor, 3) + ",";
  json += "\"frequency\":" + num(frequency, 2) + ",";
  json += "\"import_kwh\":" + num(import_kwh, 3) + ",";
  json += "\"export_kwh\":" + num(export_kwh, 3) + ",";
  json += "\"min_power\":" + num(min_power, 1) + ",";
  json += "\"max_power\":" + num(max_power, 1) + ",";
  json += "\"boot_import_delta\":" + num(import_kwh - boot_import, 3) + ",";
  json += "\"boot_export_delta\":" + num(export_kwh - boot_export, 3) + ",";
  json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  return json;
}

String deviceJson() {
  String d = "\"device\":{";
  d += "\"identifiers\":[\"esp32_sdm230_modbus\"],";
  d += "\"name\":\"" DEVICE_PRETTY_NAME "\",";
  d += "\"manufacturer\":\"ESP32\",";
  d += "\"model\":\"ESP32-S3 MAX485 SDM230\",";
  d += "\"sw_version\":\"" + String(SW_VERSION) + "\"}";
  return d;
}

void publishSensorConfig(const char* id, const char* name, const char* unit,
                         const char* device_class, const char* state_class) {
  String config_topic = "homeassistant/sensor/" + String(DEVICE_NAME) + "/" + id + "/config";
  String payload = "{";
  payload += "\"name\":\"" + String(name) + "\",";
  payload += "\"unique_id\":\"" + String(DEVICE_NAME) + "_" + id + "\",";
  payload += "\"state_topic\":\"" + topic("state") + "\",";
  payload += "\"availability_topic\":\"" + topic("availability") + "\",";
  payload += "\"payload_available\":\"online\",";
  payload += "\"payload_not_available\":\"offline\",";
  payload += "\"value_template\":\"{{ value_json." + String(id) + " }}\"";
  if (strlen(unit) > 0) payload += ",\"unit_of_measurement\":\"" + String(unit) + "\"";
  if (strlen(device_class) > 0) payload += ",\"device_class\":\"" + String(device_class) + "\"";
  if (strlen(state_class) > 0) payload += ",\"state_class\":\"" + String(state_class) + "\"";
  payload += "," + deviceJson() + "}";
  mqtt.publish(config_topic.c_str(), payload.c_str(), true);
  delay(80);
}

void publishButtonConfig() {
  String config_topic = "homeassistant/button/" + String(DEVICE_NAME) + "/restart/config";
  String payload = "{";
  payload += "\"name\":\"SDM230 ESP Restart\",";
  payload += "\"unique_id\":\"" + String(DEVICE_NAME) + "_restart\",";
  payload += "\"command_topic\":\"" + topic("restart") + "\",";
  payload += "\"payload_press\":\"restart\",";
  payload += "\"availability_topic\":\"" + topic("availability") + "\",";
  payload += "\"payload_available\":\"online\",";
  payload += "\"payload_not_available\":\"offline\",";
  payload += deviceJson() + "}";
  mqtt.publish(config_topic.c_str(), payload.c_str(), true);
}

void publishDiscovery() {
  publishSensorConfig("voltage", "SDM230 Voltage", "V", "voltage", "measurement");
  publishSensorConfig("current", "SDM230 Current", "A", "current", "measurement");
  publishSensorConfig("power", "SDM230 Power", "W", "power", "measurement");
  publishSensorConfig("apparent_power", "SDM230 Apparent Power", "VA", "apparent_power", "measurement");
  publishSensorConfig("reactive_power", "SDM230 Reactive Power", "var", "reactive_power", "measurement");
  publishSensorConfig("power_factor", "SDM230 Power Factor", "", "power_factor", "measurement");
  publishSensorConfig("frequency", "SDM230 Frequency", "Hz", "frequency", "measurement");
  publishSensorConfig("import_kwh", "SDM230 Import", "kWh", "energy", "total_increasing");
  publishSensorConfig("export_kwh", "SDM230 Export", "kWh", "energy", "total_increasing");
  publishSensorConfig("min_power", "SDM230 Minimum Power", "W", "power", "measurement");
  publishSensorConfig("max_power", "SDM230 Maximum Power", "W", "power", "measurement");
  publishSensorConfig("boot_import_delta", "SDM230 Import Since Boot", "kWh", "energy", "total");
  publishSensorConfig("boot_export_delta", "SDM230 Export Since Boot", "kWh", "energy", "total");
  publishSensorConfig("wifi_rssi", "SDM230 WiFi RSSI", "dBm", "signal_strength", "measurement");
  publishSensorConfig("free_heap", "SDM230 Free Heap", "B", "", "measurement");
  publishSensorConfig("uptime", "SDM230 Uptime", "s", "duration", "total_increasing");
  publishButtonConfig();
}

void mqttCallback(char* t, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  if (String(t) == topic("restart") && msg == "restart") {
    delay(500);
    ESP.restart();
  }
}

void mqttReconnect() {
  if (mqtt.connected()) return;
  String client_id = String(DEVICE_NAME) + "_" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (mqtt.connect(client_id.c_str(), MQTT_USER, MQTT_PASSWORD, topic("availability").c_str(), 1, true, "offline")) {
    mqtt.publish(topic("availability").c_str(), "online", true);
    mqtt.publish(topic("status").c_str(), "online", true);
    mqtt.subscribe(topic("restart").c_str());
    publishDiscovery();
    mqtt.publish(topic("state").c_str(), jsonState().c_str(), true);
  }
}

String htmlPage() {
  return R"HTML(
<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SDM230 V3.1</title>
<style>body{font-family:Arial;background:#101010;color:#eee;margin:0;padding:12px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:10px}.card{background:#222;padding:14px;border-radius:12px}.label{font-size:14px;color:#bbb}.value{font-size:27px;color:#00d4ff}.ok{color:#00ff88}.err{color:#ff5555}canvas{background:#222;border-radius:12px;width:100%;height:240px}button{padding:12px;border:0;border-radius:10px;background:#00d4ff;color:#000;font-weight:bold;margin:8px 4px}.small{font-size:13px;color:#aaa}</style>
</head><body><h2>ESP32-S3 SDM230 Modbus V3.1</h2><div class="grid">
<div class="card"><div class="label">Status</div><div id="status" class="value">---</div></div>
<div class="card"><div class="label">Voltage</div><div id="voltage" class="value">---</div></div>
<div class="card"><div class="label">Current</div><div id="current" class="value">---</div></div>
<div class="card"><div class="label">Power</div><div id="power" class="value">---</div></div>
<div class="card"><div class="label">Import total</div><div id="import_kwh" class="value">---</div></div>
<div class="card"><div class="label">Export total</div><div id="export_kwh" class="value">---</div></div>
<div class="card"><div class="label">WiFi RSSI</div><div id="wifi_rssi" class="value">---</div></div>
<div class="card"><div class="label">Uptime</div><div id="uptime" class="value">---</div></div>
</div><h3>Live Power Chart</h3><canvas id="chart" width="900" height="260"></canvas>
<button onclick="location.href='/json'">JSON API</button><button onclick="location.href='/csv'">CSV Export</button><button onclick="location.href='/discovery'">Send Discovery</button><button onclick="fetch('/restart')">Restart ESP</button>
<p class="small">Firmware 3.1.0 | OTA hostname: esp32-sdm230</p>
<script>
let data=[];function fmt(v,d,u){if(v===null||isNaN(v))return'NaN';return Number(v).toFixed(d)+' '+u}function uptime(s){if(s===null||isNaN(s))return'NaN';let h=Math.floor(s/3600),m=Math.floor((s%3600)/60);return h+'h '+m+'m '+(s%60)+'s'}function draw(){let c=document.getElementById('chart'),x=c.getContext('2d');x.clearRect(0,0,c.width,c.height);x.strokeStyle='#444';x.beginPath();for(let i=0;i<6;i++){let y=20+i*40;x.moveTo(40,y);x.lineTo(c.width-10,y)}x.stroke();if(data.length<2)return;let mn=Math.min(...data),mx=Math.max(...data);if(mx-mn<10){mx+=5;mn-=5}x.strokeStyle='#00d4ff';x.lineWidth=3;x.beginPath();for(let i=0;i<data.length;i++){let px=40+i*((c.width-60)/(data.length-1));let py=c.height-30-((data[i]-mn)/(mx-mn))*(c.height-60);if(i==0)x.moveTo(px,py);else x.lineTo(px,py)}x.stroke();x.fillStyle='#eee';x.font='14px Arial';x.fillText('Max: '+mx.toFixed(0)+' W',45,20);x.fillText('Min: '+mn.toFixed(0)+' W',45,c.height-12)}async function update(){try{let r=await fetch('/json'),j=await r.json();status.innerHTML=j.status;status.className='value '+(j.status=='OK'?'ok':'err');voltage.innerHTML=fmt(j.voltage,1,'V');current.innerHTML=fmt(j.current,3,'A');power.innerHTML=fmt(j.power,1,'W');import_kwh.innerHTML=fmt(j.import_kwh,3,'kWh');export_kwh.innerHTML=fmt(j.export_kwh,3,'kWh');wifi_rssi.innerHTML=fmt(j.wifi_rssi,0,'dBm');uptime.innerHTML=uptime(j.uptime);if(j.power!==null){data.push(j.power);if(data.length>180)data.shift();draw()}}catch(e){}}setInterval(update,1000);update();
</script></body></html>)HTML";
}

String textNum(float value, int decimals) {
  if (isnan(value)) return "NaN";
  return String(value, decimals);
}

void setupWiFi() {
#if USE_STATIC_IP
  IPAddress local_ip(STATIC_IP_ADDR);
  IPAddress gateway(GATEWAY_IP_ADDR);
  IPAddress subnet(SUBNET_MASK_ADDR);
  IPAddress dns(DNS_IP_ADDR);
  WiFi.config(local_ip, gateway, subnet, dns);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setupWebServer() {
  server.on("/", []() { server.send(200, "text/html", htmlPage()); });
  server.on("/json", []() { server.send(200, "application/json", jsonState()); });
  server.on("/discovery", []() { mqttReconnect(); publishDiscovery(); server.send(200, "text/plain", "Home Assistant Discovery sent"); });
  server.on("/restart", []() { server.send(200, "text/plain", "Restarting ESP..."); delay(500); ESP.restart(); });
  server.on("/csv", []() {
    String csv = "Name,Value,Unit\n";
    csv += "Voltage," + textNum(voltage,1) + ",V\n";
    csv += "Current," + textNum(current,3) + ",A\n";
    csv += "Power," + textNum(power,1) + ",W\n";
    csv += "Apparent Power," + textNum(apparent_power,1) + ",VA\n";
    csv += "Reactive Power," + textNum(reactive_power,1) + ",var\n";
    csv += "Power Factor," + textNum(power_factor,3) + ",\n";
    csv += "Frequency," + textNum(frequency,2) + ",Hz\n";
    csv += "Import," + textNum(import_kwh,3) + ",kWh\n";
    csv += "Export," + textNum(export_kwh,3) + ",kWh\n";
    csv += "WiFi RSSI," + String(WiFi.RSSI()) + ",dBm\n";
    csv += "Free Heap," + String(ESP.getFreeHeap()) + ",B\n";
    server.send(200, "text/csv", csv);
  });
  server.begin();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_RS485_DE_RE, OUTPUT);
  digitalWrite(PIN_RS485_DE_RE, LOW);
  Serial2.begin(SDM230_BAUDRATE, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
  node.begin(SDM230_MODBUS_ID, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  setupWiFi();
  ArduinoOTA.setHostname("esp32-sdm230");
  ArduinoOTA.begin();
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(2048);
  setupWebServer();
  readSDM230();
  mqttReconnect();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  if (!mqtt.connected()) mqttReconnect();
  mqtt.loop();
  if (millis() - last_read_ms > 1000) {
    last_read_ms = millis();
    readSDM230();
  }
  if (millis() - last_mqtt_ms > 5000) {
    last_mqtt_ms = millis();
    if (mqtt.connected()) {
      mqtt.publish(topic("state").c_str(), jsonState().c_str(), true);
      mqtt.publish(topic("availability").c_str(), "online", true);
    }
  }
}

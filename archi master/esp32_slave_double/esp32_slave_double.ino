#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ===== WiFi =====
const char* WIFI_SSID = "PASH";
const char* WIFI_PASS = "123456789";

// ===== HiveMQ Public Broker =====
const char* MQTT_HOST = "broker.hivemq.com";
const int   MQTT_PORT = 1883;   // Public MQTT TCP port

// ===== NodeMCU LED pin =====
// NodeMCU: D1 = GPIO5 [web:145]
const int LED4_PIN = D5;        // or use 5 directly

// If your LED is wired as active LOW, set to true
const bool ACTIVE_LOW = false;

// ===== Topics =====
const char* T_CMD   = "turn/led4";
const char* T_STATE = "slave/led4";

// ===== MQTT client =====
WiFiClient espClient;
PubSubClient mqtt(espClient);

// -------- Helpers --------
void writeLed(bool on) {
  if (ACTIVE_LOW) digitalWrite(LED4_PIN, on ? LOW : HIGH);
  else            digitalWrite(LED4_PIN, on ? HIGH : LOW);
}

bool parseOnOff(String p, bool &on) {
  p.trim();
  p.toUpperCase();
  if (p == "1" || p == "ON"  || p == "TRUE")  { on = true;  return true; }
  if (p == "0" || p == "OFF" || p == "FALSE") { on = false; return true; }
  return false;
}

void publishState(bool on) {
  mqtt.publish(T_STATE, on ? "ON" : "OFF", true); // retained
}

// -------- MQTT callback --------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String t(topic);
  if (t != T_CMD) return;

  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  bool on;
  if (!parseOnOff(msg, on)) return;

  writeLed(on);
  publishState(on);
}

// -------- WiFi connect --------
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK. IP: " + WiFi.localIP().toString());
}

// -------- MQTT connect --------
void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  while (!mqtt.connected()) {
    Serial.print("MQTT connecting... ");

    // ESP8266 unique-ish id
    String clientId = "NodeMCU_LED4_" + String(ESP.getChipId(), HEX); // [web:194]

    if (mqtt.connect(clientId.c_str())) {
      Serial.println("OK");

      mqtt.subscribe(T_CMD);
      publishState(false); // initial state
    } else {
      Serial.print("FAIL state=");
      Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(LED4_PIN, OUTPUT);
  writeLed(false);

  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();
}

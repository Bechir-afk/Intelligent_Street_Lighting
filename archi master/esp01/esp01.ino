/*
 * Smart City - ESP-01 WiFi Bridge
 * Handles MQTT communication for Pico
 * Communicates with Pico via Serial
 * Sends WiFi status to Pico for LED control
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ========== WiFi Credentials ==========
const char* ssid = "PASH";
const char* password = "123456789";

// ========== MQTT Broker ==========
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// ========== Lamp ID (Change for each slave) ==========
const int LAMP_ID = 1; // *** CHANGE THIS: 1, 2, 3, or 4 ***

// ========== MQTT Topics ==========
String TOPIC_CMD;       // street/turn/led{ID}
String TOPIC_STATUS;    // street/slave/led{ID}
const char* TOPIC_LIGHT = "master/light";

// ========== Objects ==========
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ========== Variables ==========
unsigned long lastRetryTime = 0;
unsigned long lastStatusUpdate = 0;
const unsigned long RETRY_INTERVAL = 3000; // 3 seconds
const unsigned long STATUS_UPDATE_INTERVAL = 500; // Update status every 500ms
String lastWifiStatus = "";

// ========== Function Prototypes ==========
void connectWiFi();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void sendWiFiStatus();

void setup() {
  // Serial communication with Pico (GPIO1=TX, GPIO3=RX)
  Serial.begin(9600);
  delay(100);
  
  // Build topics
  TOPIC_CMD = "turn/led" + String(LAMP_ID);
  TOPIC_STATUS = "slave/led" + String(LAMP_ID);
  
  // Connect WiFi
  connectWiFi();
  
  // Setup MQTT
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
  
  if (WiFi.status() == WL_CONNECTED) {
    reconnectMQTT();
  }
}

void loop() {
  // Send WiFi status to Pico periodically
  if (millis() - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
    lastStatusUpdate = millis();
    sendWiFiStatus();
  }
  
  // Check if WiFi is disconnected and retry if needed
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastRetryTime >= RETRY_INTERVAL) {
      lastRetryTime = millis();
      Serial.println("WIFI:RETRYING");
      connectWiFi();
    }
  }
  
  // Maintain MQTT connection only if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) {
      reconnectMQTT();
    }
    mqtt.loop();
  }
  
  // Check for commands from Pico
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd.startsWith("PUB:")) {
      // Format: PUB:ON or PUB:OFF
      String status = cmd.substring(4);
      if (mqtt.connected()) {
        mqtt.publish(TOPIC_STATUS.c_str(), status.c_str());
      }
    }
  }
  
  delay(10);
}

// ========== WiFi Connection ==========
void connectWiFi() {
  Serial.println("WIFI:CONNECTING");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WIFI:CONNECTED");
  } else {
    Serial.println("WIFI:FAILED");
    lastRetryTime = millis();
  }
}

// ========== Send WiFi Status to Pico ==========
void sendWiFiStatus() {
  wl_status_t status = WiFi.status();
  String currentStatus = "";
  
  if (status == WL_CONNECTED) {
    currentStatus = "CONNECTED";
  } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
    currentStatus = "FAILED";
  } else {
    currentStatus = "CONNECTING";
  }
  
  // Only send if status changed (reduce serial traffic)
  if (currentStatus != lastWifiStatus) {
    Serial.println("WIFI:" + currentStatus);
    lastWifiStatus = currentStatus;
  }
}

// ========== MQTT Callback ==========
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  // Send to Pico via serial
  if (String(topic) == TOPIC_CMD) {
    Serial.println("CMD:" + message);
  }
  
  if (String(topic) == TOPIC_LIGHT) {
    Serial.println("LIGHT:" + message);
  }
}

// ========== MQTT Reconnection ==========
void reconnectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  int attempts = 0;
  while (!mqtt.connected() && attempts < 3) {
    Serial.println("MQTT:CONNECTING");
    
    String clientId = "Slave_LED" + String(LAMP_ID) + "_" + String(random(0xffff), HEX);
    
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("MQTT:CONNECTED");
      mqtt.subscribe(TOPIC_CMD.c_str());
      mqtt.subscribe(TOPIC_LIGHT);
      return;
    } else {
      Serial.println("MQTT:FAILED");
      attempts++;
      delay(2000);
    }
  }
}

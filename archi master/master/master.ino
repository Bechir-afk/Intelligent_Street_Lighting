/*
 * Smart City - Master ESP32
 * Reads sensors and publishes data via MQTT
 * Sensors: DHT11 (temp/humidity), LDR (light), RFID-RC522
 * Added: Buzzer on RFID scan + RGB LED WiFi status
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>

// ========== WiFi Credentials ==========
const char* ssid = "PASH";
const char* password = "123456789";

// ========== MQTT Broker ==========
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_user = "";
const char* mqtt_pass = "";

// ========== MQTT Topics ==========
const char* TOPIC_TEMP = "master/temp";
const char* TOPIC_HUM = "master/hum";
const char* TOPIC_LIGHT = "master/light";
const char* TOPIC_RFID = "master/rfid";

// ========== Pin Definitions ==========
#define DHT_PIN 4           // DHT11 data pin
#define DHT_TYPE DHT22
#define LDR_PIN 35          // LDR analog pin (ADC)
#define SS_PIN 5            // RC522 SDA/SS
#define RST_PIN 17          // RC522 RST
#define BUZZER_PIN 15       // Buzzer pin
#define RGB_R 12            // RGB Red pin
#define RGB_G 14            // RGB Green pin
#define RGB_B 27            // RGB Blue pin

// ========== Sensor Objects ==========
DHT dht(DHT_PIN, DHT_TYPE);
MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ========== Timing Variables ==========
unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 2000; // Read sensors every 2 seconds
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 3000; // Check WiFi every 3 seconds

// ========== Light Threshold ==========
const int LIGHT_THRESHOLD = 600;

// ========== Function Prototypes ==========
void setupWiFi();
void reconnectMQTT();
void readAndPublishSensors();
void checkRFID();
void setRGB(int r, int g, int b);
void activateBuzzer();
void checkWiFiStatus();

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Smart City Master ESP32 ===");
  
  // Initialize RGB LED
  pinMode(RGB_R, OUTPUT);
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);
  setRGB(0, 0, 0); // Off initially
  
  // Initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize sensors
  dht.begin();
  SPI.begin();
  rfid.PCD_Init();
  pinMode(LDR_PIN, INPUT);
  
  // Connect to WiFi
  setupWiFi();
  
  // Configure MQTT
  mqtt.setServer(mqtt_server, mqtt_port);
  
  Serial.println("Master ESP32 Ready!");
}

void loop() {
  // Check WiFi status and reconnect if needed
  checkWiFiStatus();
  
  // Maintain MQTT connection only if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) {
      reconnectMQTT();
    }
    mqtt.loop();
    
    // Read and publish sensors periodically
    if (millis() - lastSensorRead >= sensorInterval) {
      lastSensorRead = millis();
      readAndPublishSensors();
    }
    
    // Check for RFID cards
    checkRFID();
  }
}

// ========== WiFi Connection ==========
void setupWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    // Blink blue while connecting
    setRGB(0, 0, 255);
    delay(250);
    setRGB(0, 0, 0);
    delay(250);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    setRGB(0, 255, 0); // Green = connected
  } else {
    Serial.println("\nWiFi Connection Failed!");
    setRGB(255, 0, 0); // Red = failed
  }
}

// ========== Check WiFi Status ==========
void checkWiFiStatus() {
  if (millis() - lastWiFiCheck >= wifiCheckInterval) {
    lastWiFiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected! Reconnecting...");
      setRGB(255, 0, 0); // Red = failed
      delay(500);
      
      // Try to reconnect
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        // Blink blue while reconnecting
        setRGB(0, 0, 255);
        delay(250);
        setRGB(0, 0, 0);
        delay(250);
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Reconnected!");
        setRGB(0, 255, 0); // Green = connected
      } else {
        Serial.println("WiFi Reconnection Failed! Retrying in 3s...");
        setRGB(255, 0, 0); // Red = failed
      }
    } else {
      setRGB(0, 255, 0); // Keep green when connected
    }
  }
}

// ========== MQTT Reconnection ==========
void reconnectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    
    String clientId = "Master_ESP32_" + String(random(0xffff), HEX);
    
    if (mqtt.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Connected!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Retrying in 5s...");
      delay(5000);
    }
  }
}

// ========== Read and Publish Sensor Data ==========
void readAndPublishSensors() {
  // Read DHT22
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  // Read LDR
  int ldrValue = analogRead(LDR_PIN);
  String lightDecision = (ldrValue < LIGHT_THRESHOLD) ? "Dark" : "Light";
  
  // Check for valid DHT readings
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT22 read failed!");
    return;
  }
  
  // Publish Temperature
  mqtt.publish(TOPIC_TEMP, String(temperature).c_str());
  Serial.print("Published Temp: ");
  Serial.println(temperature);
  
  // Publish Humidity
  mqtt.publish(TOPIC_HUM, String(humidity).c_str());
  Serial.print("Published Humidity: ");
  Serial.println(humidity);
  
  // Publish Light Decision
  mqtt.publish(TOPIC_LIGHT, lightDecision.c_str());
  Serial.print("Published Light: ");
  Serial.print(lightDecision);
  Serial.print(" (LDR: ");
  Serial.print(ldrValue);
  Serial.println(")");
}

// ========== Check for RFID Card ==========
void checkRFID() {
  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  // Read UID
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  
  // Activate buzzer when card is scanned
  activateBuzzer();
  
  // Publish RFID UID
  mqtt.publish(TOPIC_RFID, uid.c_str());
  Serial.print("Published RFID: ");
  Serial.println(uid);
  
  // Halt PICC
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ========== Set RGB LED Color ==========
void setRGB(int r, int g, int b) {
  analogWrite(RGB_R, r);
  analogWrite(RGB_G, g);
  analogWrite(RGB_B, b);
}

// ========== Activate Buzzer ==========
void activateBuzzer() {
  Serial.println("Buzzer: RFID Scanned!");
  
  // Beep pattern: 3 short beeps
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

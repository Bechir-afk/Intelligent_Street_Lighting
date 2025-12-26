# Intelligent_Street_Lighting (Smart City IoT)

Intelligent street lighting prototype for Smart City applications using a **Master–Slaves** IoT architecture.  
An **ESP32 Master** reads sensors (LDR, DHT, RFID), publishes telemetry, and controls multiple lamp **Slave nodes** via **MQTT** through the **HiveMQ broker**. A **Web Dashboard** provides real-time monitoring and manual control, secured with **Firebase Authentication**, and logs data to **SQLite** for history/analysis. 
---

## System overview

### Nodes
- **Master (ESP32):**
  - Reads **LDR** (day/night), **DHT** (temperature/humidity), **RFID** (UID).
  - Publishes telemetry and (in AUTO mode) sends commands to slaves. 

- **Slaves (Lamp posts):**
  - **ESP32 slaves**: connect directly to MQTT and drive LEDs locally.
  - **Pico + ESP-01 slave**: ESP‑01 handles Wi‑Fi/MQTT, Pico drives LEDs; communication between them via **UART @ 9600 baud**. 

### Cloud / App
- **HiveMQ MQTT Broker:** centralizes all publish/subscribe traffic between Master, Slaves, and Dashboard. 
- **Web Dashboard (HTML/CSS/JS):** real-time UI + manual ON/OFF/AUTO control (WebSocket). 
- **Firebase Authentication:** protects dashboard access (users/roles + RFID association). 
- **SQLite database:** stores sensor history + lamp states (logs). 

---

## Features
- Real-time telemetry: temperature, humidity, light state (day/night), RFID UID events. 
- Lamp control modes:
  - **ON**: force ON
  - **OFF**: force OFF
  - **AUTO**: Master decides based on ambient light (LDR) 
- Per-lamp manual control from the dashboard. 
- Secure access using Firebase Auth (+ optional RFID-based access flow). 
- Local history storage with SQLite for reporting/analytics. 

---

## MQTT Topics

### Master → Broker (telemetry)
- `master/temp`
- `master/hum`
- `master/light`
- `master/rfid` 

### Commands → Slaves
- `turn/ledX`  
Payload examples: `ON`, `OFF`, `AUTO` (or `1` / `0` depending on firmware). 

### Slaves → Broker (feedback)
- `slave/ledX`  
Payload: `ON` / `OFF`. 

> Replace `X` with the lamp ID (example: `turn/led4`, `slave/led4`). 

---

## Workflow (how it works)
1. Master reads sensors and publishes telemetry to HiveMQ. 
2. **AUTO mode:** Master decides ON/OFF based on LDR and publishes `turn/ledX`. 
3. **MANUAL mode:** Dashboard publishes `turn/ledX` commands to HiveMQ. 
4. Slaves execute the command and publish the new state to `slave/ledX`.
5. Dashboard updates the UI and logs data to SQLite. 

---

## Getting started

### Prerequisites
- Arduino IDE / PlatformIO for ESP32/ESP8266 firmware
- Raspberry Pi Pico toolchain (Arduino-Pico or MicroPython, depending on your implementation) 
- MQTT client for testing (MQTT Explorer / MQTT Dash / mosquitto_pub/sub)
- Firebase project (Authentication enabled + database structure for workers/users)

### 1) Configure the broker
- Set your MQTT host (HiveMQ) in Master/Slave firmware and in the dashboard connection.

### 2) Flash the devices
- Flash **ESP32 Master** (configure Wi‑Fi + MQTT + sensor pins).
- Flash each **Slave** (configure lamp ID + its topics).
- For **Pico + ESP‑01**: flash both firmwares and ensure UART is wired and set to **9600 baud**.

### 3) Run the dashboard
- Serve the dashboard files with any static server.
- Configure:
  - MQTT WebSocket endpoint
  - Firebase config (API key, project id, etc.) 

### 4) Test quickly (example)

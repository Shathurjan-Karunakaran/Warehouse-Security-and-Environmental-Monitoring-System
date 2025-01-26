# 🚀 Warehouse Security & Environmental Monitoring System

ESP32-based Embedded Project

---

## 🌐 Project Overview

**Master-Slave Architecture:** The project uses two ESP32 boards:

- **Slave Circuit:** ESP32 with DHT22 (Temperature & Humidity), MQ135 (Air Quality), HC-SR501 (Motion Sensor) 
- **Master Circuit:** ESP32 with RFID, Buzzer, OLED Display, and LED

💡 **Note:** Both circuits are powered using phone chargers and communicate via Wi-Fi or LoRa (as an alternative).

---

## 🔧 Features

- ⏳ Real-time sensor data collection and transmission to the cloud using MQTT protocol
- 🌐 Master ESP32 acts as both Wi-Fi access point and client
- 🔑 Secure access control with RFID
- 📝 Visual feedback with OLED display

---

## 📡 Communication

- **Wi-Fi:** Slave sends sensor data to Master ESP32 (range: 40-60 meters)
- **MQTT:** Data sent to the cloud for GUI visualization
- **LoRa (optional):** Range up to 100 meters

---

## 🔹 Protocols

- **I2C:** Communication between Master ESP32 and OLED display
- **SPI:** Communication between Master ESP32 and RFID module

---

## 🔋 Circuit Design

**Master and Slave circuits have distinct designs:**

- **Slave Circuit:** Connects DHT22, MQ135, and HC-SR501 sensors
- **Master Circuit:** Connects RFID, LED, Buzzer, and OLED display
- **Power source:** Phone chargers for each circuit

---

## 🔄 Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/your-repo-name/esp32-warehouse-security
   ```

2. Install required libraries:
   ```bash
   pip install paho-mqtt
   ```

3. Upload the code to ESP32 boards via Arduino IDE or PlatformIO.

---

## 🎨 Circuit Diagram

*(Placeholder for Circuit Diagram)*

---

## 📚 Learn More

Visit our **[GitHub Repository](https://github.com/your-repo-name)** for detailed documentation, code, and setup instructions.

---

🌐 **Created by Team ESP32 Innovators**

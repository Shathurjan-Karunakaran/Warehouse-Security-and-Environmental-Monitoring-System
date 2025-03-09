
# Warehouse Security and Environmental Monitoring System 🚀

## Introduction 📘

The Warehouse Security and Environmental Monitoring System integrates security and environmental monitoring using a master-slave ESP32 architecture. It tracks temperature, humidity, gas levels, and motion while providing real-time alerts and cloud connectivity. This scalable IoT solution is designed to improve warehouse safety and operational efficiency.

## Objective 🎯

- Prevent unauthorized access with motion and access sensors.
- Monitor temperature, humidity, gas, and smoke levels.
- Provide real-time alerts through alarms or remote notifications.
- Enable remote monitoring via cloud integration.
- Ensure efficient data processing using ESP32 architecture.

## Problem Statement ⚠️

Warehouses are critical supply chain hubs that store valuable goods. However, they face risks from environmental fluctuations (temperature, humidity, air quality) and unauthorized access—leading to potential gas leaks, fire hazards, and other safety issues. Existing monitoring systems are often expensive, inefficient, or lack real-time, centralized monitoring. This makes manual supervision error-prone and costly. An affordable, reliable, and integrated solution is therefore essential.

## Solution 💡

Our system combines environmental monitoring with access control and real-time data visualization. Using a master-slave architecture, slave ESP32 modules collect sensor data (temperature, humidity, gas, and motion), while the master ESP32 processes the data, manages RFID-based access, and communicates with the cloud via MQTT. This design ensures scalability, centralized control, and cost-effectiveness, providing a robust framework for warehouse safety.

## Key Features ✅

- **Hybrid Master-Slave Architecture:** Master ESP32 acts as both a Wi-Fi access point and a cloud-connected node.
- **Integrated Monitoring:** Combines environmental sensors and security features (motion detection and RFID access control).
- **Cloud Connectivity & Real-Time Visualization:** Uses MQTT for lightweight, real-time data transmission and remote monitoring via a GUI.
- **Modular & Scalable Design:** Easily expandable with additional slave devices and sensors.
- **Efficient Communication:** Utilizes I2C for the OLED display and SPI for the RFID module.

## Circuit Components 🔌

### Slave Circuit Components

<div align="center">

<table style="width:80%; border-collapse: collapse;">
  <tr>
    <th style="border: 1px solid #ddd; padding: 8px; text-align: center; background-color: #f2f2f2;">Component</th>
    <th style="border: 1px solid #ddd; padding: 8px; text-align: center; background-color: #f2f2f2;">Quantity</th>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">ESP32 WROOM 32</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">1</td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">DHT22 Sensor</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">1</td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">MQ135 Sensor</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">1</td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">HC-SR501 PIR Sensor</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">1</td>
  </tr>
</table>
</div>

### Master Circuit Components

<div align="center">

<table style="width:80%; border-collapse: collapse; margin-top: 15px;">
  <tr>
    <th style="border: 1px solid #ddd; padding: 8px; text-align: center; background-color: #f2f2f2;">Component</th>
    <th style="border: 1px solid #ddd; padding: 8px; text-align: center; background-color: #f2f2f2;">Quantity</th>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">ESP32 WROOM 32</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">1</td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">RFID Module</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">1</td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">OLED Display</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">1</td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">Buzzer</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">1</td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">LED</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">1</td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">220Ω Resistor</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">3</td>
  </tr>
</table>
</div>

## Communication Protocols 📡

<div align="center">

| **Protocol** | **Description**                                    | **Advantages**                                                  |
|:------------:|:--------------------------------------------------:|:---------------------------------------------------------------:|
| I2C         | Communication between OLED display and ESP32       | Simplifies wiring and ensures efficient data transfer           |
| SPI         | Communication with RFID module                     | Provides high-speed data transmission                           |
| Wi-Fi       | Communication between master and slave nodes       | Reliable for short-range, real-time monitoring within 40-60 m    |
| MQTT        | Cloud data transfer                                | Lightweight, secure, and enables real-time updates               |

</div>

## Circuit Diagram and Protocols 🔍

Below are the circuit diagrams for both the slave and master circuits along with the communication protocols used.

<div align="center">

![s](https://github.com/user-attachments/assets/87889755-6e99-4d3f-b39e-10c78ce8977b)


![m](https://github.com/user-attachments/assets/8b2a2de1-3ee3-49c2-9db5-89e86515f6f0)

</div>

## Simulation 🖥️

A simulation of the system setup has been performed to verify sensor readings and communication flows.

<div align="center">

![Slave 1](https://github.com/user-attachments/assets/1ad8cc4a-7acb-4438-9dcf-74f4de106fbc)

![Master 1](https://github.com/user-attachments/assets/57c7f10f-bec4-4ce4-a5b5-7c30c50d29e3)
</div>

## Phase One of Execution 🏁

Phase One involves the initial hardware setup, sensor calibration, and establishing the master-slave communication link.

<div align="center">
<!-- Random image for Phase One of Execution -->
![Phase One Execution](https://via.placeholder.com/400x300?text=Phase+One+Execution)
</div>

## Software Integration 💻

- **Programming Language:** C
- **Mobile/Cloud Application:** Thingsboard (for real-time visualization and remote monitoring)

## Budget 💰

<div align="center">

| **Component**                          | **Quantity** | **Price (LKR)**          |
|:--------------------------------------:|:------------:|:------------------------:|
| ESP32 WROOM 32                         |      2       | 1900 × 2 = **3800**      |
| DHT22 Sensor                           |      1       | **400**                |
| MQ135 Sensor                           |      1       | **370**                |
| HC-SR501 PIR Sensor                    |      1       | **250**                |
| RFID Module                            |      1       | **270**                |
| 0.96-inch 128X64 OLED Display Module   |      1       | **640**                |
| LED 3V                                 |      1       | **40**                 |
| 220Ω Resistor                          |      3       | **60**                 |
| Buzzer 3V                              |      1       | **40**                 |
| Jumper Wires                           |     25       | **250**                |
| Breadboard                             |      2       | 350 × 2 = **700**       |
| **TOTAL**                              |      -       | **Rs. 6820**           |

</div>

## Timeline ⏱️

### Timeline Table

<div align="center">

| **Task**                       | **Timeline**   |
|:------------------------------:|:--------------:|
| Project Proposal               | Week 4         |
| Hardware Setup                 | Weeks 5 - 6    |
| Mid-Evaluation Preparation     | Week 7         |
| Mid-Evaluation                 | Week 9         |
| Full System Integration        | Weeks 10 - 12  |
| Software Development           | Weeks 13 - 15  |
| Testing & Refining             | Weeks 14 - 15  |
| Final Evaluation Preparation   | Week 14       |
| Final Evaluation               | Week 15        |

</div>

### Timeline Flowchart

```mermaid
flowchart TD
  A["Project Proposal (Week 4)"] --> B["Hardware Setup (Weeks 5-6)"]
  B --> C["Mid-Evaluation Preparation (Week 7)"]
  C --> D["Mid-Evaluation (Week 9)"]
  D --> E["Full System Integration (Weeks 10-12)"]
  E --> F["Software Development (Weeks 13-15)"]
  F --> G["Testing & Refining (Weeks 14-15)"]
  G --> H["Final Evaluation Preparation (Week 14)"]
  H --> I["Final Evaluation (Week 15)"]

  classDef timeline fill:#B3E5FC,stroke:#00796b,stroke-width:2px, color:#000, rx:15, ry:15;
  class A,B,C,D,E,F,G,H,I timeline;

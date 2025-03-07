# Warehouse Security and Environmental Monitoring System 🚀

## 1. Introduction 📘

The Warehouse Security and Environmental Monitoring System integrates security and environmental monitoring using a master-slave ESP32 architecture. It tracks temperature, humidity, gas levels, and motion while providing real-time alerts and cloud connectivity. This scalable IoT solution is designed to improve warehouse safety and operational efficiency.

## 2. Objective 🎯

- Prevent unauthorized access with motion and access sensors.
- Monitor temperature, humidity, gas, and smoke levels.
- Provide real-time alerts through alarms or remote notifications.
- Enable remote monitoring via cloud integration.
- Ensure efficient data processing using ESP32 architecture.

## 3. Problem Statement ⚠️

Warehouses are critical supply chain hubs that store valuable goods. However, they face risks from environmental fluctuations (temperature, humidity, air quality) and unauthorized access—leading to potential gas leaks, fire hazards, and other safety issues. Existing monitoring systems are often expensive, inefficient, or lack real-time, centralized monitoring. This makes manual supervision error-prone and costly. An affordable, reliable, and integrated solution is therefore essential.

## 4. Solution 💡

Our system combines environmental monitoring with access control and real-time data visualization. Using a master-slave architecture, slave ESP32 modules collect sensor data (temperature, humidity, gas, and motion), while the master ESP32 processes the data, manages RFID-based access, and communicates with the cloud via MQTT. This design ensures scalability, centralized control, and cost-effectiveness, providing a robust framework for warehouse safety.

## 5. Key Features ✅

- **Hybrid Master-Slave Architecture:** Master ESP32 acts as both a Wi-Fi access point and a cloud-connected node.
- **Integrated Monitoring:** Combines environmental sensors and security features (motion detection and RFID access control).
- **Cloud Connectivity & Real-Time Visualization:** Uses MQTT for lightweight, real-time data transmission and remote monitoring via a GUI.
- **Modular & Scalable Design:** Easily expandable with additional slave devices and sensors.
- **Efficient Communication:** Utilizes I2C for the OLED display and SPI for the RFID module.

## 6. Circuit Components 🔌

### Slave Circuit Components

<table style="width:100%; border-collapse: collapse;">
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

### Master Circuit Components

<table style="width:100%; border-collapse: collapse; margin-top: 15px;">
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

## 7. Communication Protocols 📡

The following table summarizes the communication protocols used in the system, along with their descriptions and advantages.

| **Protocol** | **Description**                                    | **Advantages**                                                  |
|:------------:|:--------------------------------------------------:|:---------------------------------------------------------------:|
| I2C         | Communication between OLED display and ESP32       | Simplifies wiring and ensures efficient data transfer           |
| SPI         | Communication with RFID module                     | Provides high-speed data transmission                           |
| Wi-Fi       | Communication between master and slave nodes       | Reliable for short-range, real-time monitoring within 40-60 m    |
| MQTT        | Cloud data transfer                                | Lightweight, secure, and enables real-time updates               |

## 8. Circuit Diagram and Protocols 🔍

Below are the circuit diagrams for both the slave and master circuits along with the communication protocols used.

<!-- Random image for Slave Circuit Diagram -->
![Slave Circuit Diagram](https://via.placeholder.com/400x300?text=Slave+Circuit+Diagram)

<!-- Random image for Master Circuit Diagram -->
![Master Circuit Diagram](https://via.placeholder.com/400x300?text=Master+Circuit+Diagram)

## 9. Simulation 🖥️

A simulation of the system setup has been performed to verify sensor readings and communication flows.

<!-- Random image for Simulation -->
![Simulation](https://via.placeholder.com/400x300?text=Simulation)

## 10. Phase One of Execution 🏁

Phase One involves the initial hardware setup, sensor calibration, and establishing the master-slave communication link.

<!-- Random image for Phase One of Execution -->
![Phase One Execution](https://via.placeholder.com/400x300?text=Phase+One+Execution)

## 11. Software Integration 💻

- **Programming Language:** C++
- **Mobile/Cloud Application:** Thingsboard (for real-time visualization and remote monitoring)

## 12. Budget 💰

The following table outlines the estimated budget for the components (prices in LKR):

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

## 13. Timeline ⏱️

Below is the project timeline represented as both a table and a flowchart.

### Timeline Table

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

### Timeline Flowchart

```mermaid
flowchart TD
  classDef timeline fill:#e0f7fa,stroke:#00796b,stroke-width:2px;
  A[Project Proposal (Week 4)]:::timeline --> B[Hardware Setup (Weeks 5-6)]:::timeline
  B --> C[Mid-Evaluation Preparation (Week 7)]:::timeline
  C --> D[Mid-Evaluation (Week 9)]:::timeline
  D --> E[Full System Integration (Weeks 10-12)]:::timeline
  E --> F[Software Development (Weeks 13-15)]:::timeline
  F --> G[Testing & Refining (Weeks 14-15)]:::timeline
  G --> H[Final Evaluation Preparation (Week 14)]:::timeline
  H --> I[Final Evaluation (Week 15)]:::timeline

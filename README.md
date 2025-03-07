# Warehouse Security and Environmental Monitoring System 🚀

## Project Website 🌐

<div align="center">
[**Visit the Project Website**](https://warehousesecuritymonitor.my.canva.site/)
</div>

## Introduction 📘

- **Integrated System:** Combines security & environmental monitoring using a master-slave ESP32 architecture.
- **Multi-Sensor Tracking:** Monitors temperature, humidity, gas levels, and motion.
- **Real-Time Alerts:** Provides immediate notifications via cloud connectivity.
- **Scalable & Efficient:** Designed to improve warehouse safety and operational efficiency.

## Objective 🎯

- Prevent unauthorized access using motion and access sensors.
- Monitor critical environmental parameters (temperature, humidity, gas, and smoke).
- Deliver real-time alerts through alarms or remote notifications.
- Enable remote monitoring via cloud integration.
- Process sensor data efficiently using ESP32 architecture.

## Problem Statement ⚠️

- **High Risk Environment:** Warehouses store valuable goods and face environmental risks.
- **Environmental Fluctuations:** Susceptible to changes in temperature, humidity, and air quality.
- **Security Threats:** Risk of unauthorized access, gas leaks, and fire hazards.
- **Inefficient Existing Systems:** Current solutions are often expensive, lack real-time monitoring, or require manual supervision.

## Solution 💡

- **Combined Monitoring:** Integrates environmental sensing with access control.
- **Master-Slave Architecture:** 
  - **Slave Nodes:** Collect sensor data (temperature, humidity, gas, motion).
  - **Master Node:** Processes data, manages RFID access, and communicates via MQTT.
- **Scalability:** Easily expandable for larger warehouses.
- **Cost-Effective & Centralized:** Offers a robust, affordable solution for warehouse safety.

## Key Features ✅

- **Hybrid Master-Slave Architecture:** Master ESP32 acts as a Wi-Fi access point and cloud-connected node.
- **Integrated Monitoring:** Combines environmental sensors and security features (motion detection and RFID access).
- **Cloud Connectivity:** Uses MQTT for lightweight, real-time data transmission.
- **Modular & Scalable Design:** Easily expandable with additional slave devices and sensors.
- **Efficient Communication:** Uses I2C for OLED display and SPI for the RFID module.

## Circuit Components 🔌

### Slave Circuit Components (Infographics)

<div align="center">

<table style="width:80%; border-collapse: collapse;">
  <tr>
    <th style="border: 1px solid #ddd; padding: 8px; text-align: center; background-color: #f2f2f2;">Component</th>
    <th style="border: 1px solid #ddd; padding: 8px; text-align: center; background-color: #f2f2f2;">Infographic</th>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">ESP32 WROOM 32</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![ESP32](https://via.placeholder.com/50?text=ESP32)
    </td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">DHT22 Sensor</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![DHT22](https://via.placeholder.com/50?text=DHT22)
    </td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">MQ135 Sensor</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![MQ135](https://via.placeholder.com/50?text=MQ135)
    </td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">HC-SR501 PIR Sensor</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![PIR](https://via.placeholder.com/50?text=PIR)
    </td>
  </tr>
</table>
</div>

### Master Circuit Components (Infographics)

<div align="center">

<table style="width:80%; border-collapse: collapse; margin-top: 15px;">
  <tr>
    <th style="border: 1px solid #ddd; padding: 8px; text-align: center; background-color: #f2f2f2;">Component</th>
    <th style="border: 1px solid #ddd; padding: 8px; text-align: center; background-color: #f2f2f2;">Infographic</th>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">ESP32 WROOM 32</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![ESP32](https://via.placeholder.com/50?text=ESP32)
    </td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">RFID Module</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![RFID](https://via.placeholder.com/50?text=RFID)
    </td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">OLED Display</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![OLED](https://via.placeholder.com/50?text=OLED)
    </td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">Buzzer</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![Buzzer](https://via.placeholder.com/50?text=Buzzer)
    </td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">LED</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![LED](https://via.placeholder.com/50?text=LED)
    </td>
  </tr>
  <tr>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">220Ω Resistor</td>
    <td style="border: 1px solid #ddd; padding: 8px; text-align: center;">
      ![Resistor](https://via.placeholder.com/50?text=Resistor)
    </td>
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
<!-- Random image for Slave Circuit Diagram -->
![Slave Circuit Diagram](https://via.placeholder.com/400x300?text=Slave+Circuit+Diagram)

<!-- Random image for Master Circuit Diagram -->
![Master Circuit Diagram](https://via.placeholder.com/400x300?text=Master+Circuit+Diagram)
</div>

## Simulation 🖥️

A simulation of the system setup has been performed to verify sensor readings and communication flows.

<div align="center">
<!-- Random image for Simulation -->
![Simulation](https://via.placeholder.com/400x300?text=Simulation)
</div>

## Phase One of Execution 🏁

Phase One involves the initial hardware setup, sensor calibration, and establishing the master-slave communication link.

<div align="center">
<!-- Random image for Phase One of Execution -->
![Phase One Execution](https://via.placeholder.com/400x300?text=Phase+One+Execution)
</div>

## Software Integration 💻

- **Programming Language:** C++
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

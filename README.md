<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Warehouse Security & Environmental Monitoring System</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      line-height: 1.6;
      background-color: #f4f4f9;
      margin: 0;
      padding: 0;
    }
    header {
      background-color: #0077cc;
      color: white;
      padding: 1rem 0;
      text-align: center;
    }
    .content {
      max-width: 800px;
      margin: 2rem auto;
      padding: 1rem;
      background: white;
      border-radius: 10px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
    }
    .section {
      margin-bottom: 1.5rem;
    }
    .section h2 {
      color: #0077cc;
      border-bottom: 2px solid #0077cc;
      padding-bottom: 0.5rem;
    }
    code {
      display: block;
      background: #f9f9f9;
      padding: 1rem;
      border-radius: 8px;
      overflow-x: auto;
    }
    .diagram {
      text-align: center;
      margin: 1.5rem 0;
    }
    footer {
      text-align: center;
      margin-top: 2rem;
      font-size: 0.9rem;
      color: #555;
    }
    .highlight {
      background: #eaf4fe;
      border-left: 4px solid #0077cc;
      padding: 0.5rem;
      margin: 1rem 0;
    }
  </style>
</head>
<body>
  <header>
    <h1>Warehouse Security & Environmental Monitoring System</h1>
    <p>ESP32-based Embedded Project</p>
  </header>
  <div class="content">
    <div class="section">
      <h2>Project Overview</h2>
      <p><strong>Master-Slave Architecture:</strong> The project uses two ESP32 boards:</p>
      <ul>
        <li><strong>Slave Circuit:</strong> ESP32 with DHT22 (Temperature & Humidity), MQ135 (Air Quality), HC-SR501 (Motion Sensor)</li>
        <li><strong>Master Circuit:</strong> ESP32 with RFID, Buzzer, OLED Display, and LED</li>
      </ul>
      <div class="highlight">
        Both circuits are powered using phone chargers and communicate via Wi-Fi or LoRa (as an alternative).</div>
    </div>
    <div class="section">
      <h2>Features</h2>
      <ul>
        <li>Real-time sensor data collection and transmission to the cloud using MQTT protocol</li>
        <li>Master ESP32 acts as both Wi-Fi access point and client</li>
        <li>Secure access control with RFID</li>
        <li>Visual feedback with OLED display</li>
      </ul>
    </div>
    <div class="section">
      <h2>Communication</h2>
      <ul>
        <li><strong>Wi-Fi:</strong> Slave sends sensor data to Master ESP32 (range: 40-60 meters)</li>
        <li><strong>MQTT:</strong> Data sent to the cloud for GUI visualization</li>
        <li><strong>LoRa (optional):</strong> Range up to 100 meters</li>
      </ul>
    </div>
    <div class="section">
      <h2>Protocols</h2>
      <ul>
        <li><strong>I2C:</strong> Communication between Master ESP32 and OLED display</li>
        <li><strong>SPI:</strong> Communication between Master ESP32 and RFID module</li>
      </ul>
    </div>
    <div class="section">
      <h2>Circuit Design</h2>
      <div class="diagram">
        <img src="https://via.placeholder.com/600x300" alt="Circuit Diagram Placeholder" />
      </div>
      <p>Master and Slave circuits have distinct designs:</p>
      <ul>
        <li><strong>Slave Circuit:</strong> Connects DHT22, MQ135, and HC-SR501 sensors</li>
        <li><strong>Master Circuit:</strong> Connects RFID, LED, Buzzer, and OLED display</li>
        <li>Power source: Phone chargers for each circuit</li>
      </ul>
    </div>
    <div class="section">
      <h2>Getting Started</h2>
      <p>Clone the repository:</p>
      <code>git clone https://github.com/your-repo-name/esp32-warehouse-security</code>
      <p>Install required libraries:</p>
      <code>pip install paho-mqtt</code>
    </div>
  </div>
  <footer>
    <p>Created by Team ESP32 Innovators | <a href="https://github.com/your-repo-name">GitHub Repository</a></p>
  </footer>
</body>
</html>

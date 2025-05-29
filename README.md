# Weather IoT WiFi Transceiver

This project is the WiFi-based receiver and logger for the Weather IoT Sensor system. It runs on an ESP32 board and is responsible for receiving weather and GPS data from a sensor node through UART, and then transmitting the data to a server or cloud endpoint for storage and analysis.

## Features

- ESP32-based data receiver and uploader
- WiFi connectivity
- MQTT support
- Modular code structure with PlatformIO

## Hardware Requirements

- ESP32-01S board
- ESP32-01S adapter (AMS1117 based)
- Optional: Step-down buck converter (MP1584) to prevent AMS1117's overheating
- Optional: Electrolic capacitor (I used 100 uF) to handle ESP32's power spikes 

## Software Requirements

- [PlatformIO](https://platformio.org/) (for building and flashing the firmware)
- Arduino framework for ESP32 (automatically handled by PlatformIO)

## Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/scuya2050/weather-iot-wifi.git
   cd weather-iot-wifi
   ```

2. Open the project using PlatformIO (e.g., in VS Code).

3. Connect your ESP32 board via USB.

4. Upload the code:

   ```bash
   pio run --target upload
   ```

## Configuration

Update WiFi credentials and server endpoints inside the `main.cpp` file or create a `secrets.h`. Additionally, it is recommended to save the certificates to connect to the MQTT broker (if needed) in a known location or in a folder inside the Project directory

## Usage

- The ESP32 connects to a WiFi network based on the configuration provided. Then it connects to the MQTT broker.
- Upon successful connection, it requests data from the sensor node.
- After receiving the response, it publishes to the configured MQTT topic.
- Given that the ESP32 can get the current date and time through a NTP Server, it can aid the sensor node to determine the timestamp of the measurements

## Project Structure

```
weather-iot-wifi/
├── include/            # Header files
├── lib/                # External libraries (not used)
├── src/                # Main source code
│   └── main.cpp
├── test/               # Tests (optional, not used)
├── platformio.ini      # PlatformIO project config
└── .vscode/            # VSCode config (optional)
```

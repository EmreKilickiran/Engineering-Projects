# Cyber-Physical Framework for Real-Time Industrial Asset Monitoring

A low-cost IoT-based real-time industrial monitoring system enabling continuous machine parameter tracking and real-time carbon footprint computation in compliance with European environmental standards. Developed during an R&D internship at **Canovate Electronics**, Istanbul.

## Overview

The system integrates embedded hardware with a client-server software architecture for automated sensor data acquisition, processing, and alert generation. Three firmware modules run on the Arduino Opta PLC, communicating with energy analyzers via Modbus RS485 and transmitting encoded data to a local Flask server.

### System Architecture

```
┌───────────────────────────────────────────────────────────┐
│  PHYSICAL LAYER                                           │
│                                                           │
│  Finder 6M Energy Analyzers × 3                           │
│  (Modbus addresses: 2, 3, 7)                              │
│  Sensors: Voltage, Current, Power Factor, THD,            │
│           Energy (pos/neg), Frequency                     │
└──────────────────┬────────────────────────────────────────┘
                   │ Modbus RS485
┌──────────────────▼────────────────────────────────────────┐
│  EMBEDDED LAYER — Arduino Opta PLC (ARM Cortex-M7)        │
│                                                           │
│  ┌──────────────────────┐  ┌──────────────────────┐       │
│  │ sensor_acquisition   │  │ timestamp_encoder    │       │
│  │ .ino                 │  │ .ino                 │       │
│  │                      │  │                      │       │
│  │ • Modbus polling     │  │ • NTP time sync      │       │
│  │ • 10s auto-baseline  │  │ • Date+time → hex    │       │
│  │ • Threshold alerting │  │ • JSON transmission  │       │
│  │ • Float → hex encode │  │                      │       │
│  │ • HTTP POST to server│  │                      │       │
│  └──────────────────────┘  └──────────────────────┘       │
│                                                           │
│  ┌──────────────────────┐                                 │
│  │ web_config_interface │                                 │
│  │ .ino                 │                                 │
│  │                      │                                 │
│  │ • 8 input configs    │                                 │
│  │ • 16 device addresses│                                 │
│  │ • EEPROM persistence │                                 │
│  │ • HTML/JS interface  │                                 │
│  └──────────────────────┘                                 │
└──────────────────┬────────────────────────────────────────┘
                   │ Ethernet (HTTP POST)
┌──────────────────▼─────────────────────────────────────────┐
│  SERVER LAYER — Flask (Python)                             │
│                                                            │
│  • Hex payload decoding                                    │
│  • Parameter visualization                                 │
│  • Threshold-triggered email alerts                        │
│  • Data logging                                            │
└────────────────────────────────────────────────────────────┘
```

## Firmware Modules

### `sensor_acquisition.ino` - Core Data Acquisition & Alerting
The main firmware module. Polls three Finder 6M analyzers via Modbus at continuous sampling rates, applies automatic baseline calibration during the first 10 seconds, and monitors sensor values against configurable tolerance thresholds. Data is encoded as IEEE 754 hexadecimal for compact RS485 transmission. Alerts trigger immediate data transmission to the server.

**Monitored parameters per analyzer:** Voltage RMS, Current RMS, Active/Reactive/Apparent Power, Power Factor, Frequency, THD, Positive/Negative Energy.

### `timestamp_encoder.ino` - NTP Time Synchronization & Hex Encoding
Acquires real-time timestamps from NTP servers (pool.ntp.org, GMT+3), encodes date (DDMMYY) and time (HHMMSS) as hexadecimal integers, and transmits the combined timestamp as JSON to the Flask server.

### `web_config_interface.ino` - Remote Configuration Interface
Serves an embedded HTML/JavaScript web interface for remote configuration of input types (Counter/Analog/PWM), Modbus device assignments (Analyzer/Temperature/Pressure), and per-device alert thresholds (min/max for Watt, Voltage, Current). All settings are persisted in EEPROM.

## Hardware

| Component | Specification |
|-----------|---------------|
| Controller | Arduino Opta PLC (ARM Cortex-M7) |
| Analyzers | Finder 6M × 3 (Modbus RS485) |
| Network | W5500 Ethernet module |
| Communication | RS485 (Modbus) + Ethernet (HTTP) |

## Repository Structure

```
.
├── README.md
└── firmware/
    ├── sensor_acquisition.ino      # Modbus polling + alerting + hex encoding
    ├── timestamp_encoder.ino       # NTP timestamp hex encoding
    └── web_config_interface.ino    # Web-based parameter configuration
```

## Dependencies (Arduino Libraries)

- `Finder6M` - Finder energy analyzer Modbus driver
- `Ethernet` / `EthernetUdp` - W5500 networking
- `ArduinoHttpClient` - HTTP client for POST requests
- `ArduinoJson` - JSON serialization
- `NTPClient` / `TimeLib` - NTP time synchronization
- `EEPROM` - Persistent configuration storage

## Results

The full pipeline was validated on a real-time demo setup, confirming end-to-end functionality from sensor acquisition to automated alerting.

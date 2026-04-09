# Real-Time Signal Processing and Embedded Control Algorithms for a Custom UAV

Custom firmware in C for a 7-channel RC aircraft control system, implementing real-time PWM signal processing and wireless communication on Arduino. Self-directed independent engineering project.

## Overview

Independently designed and built a complete RC transmitter-receiver system from scratch, replacing commercial off-the-shelf controllers. The system reads joystick inputs, calibrates them with dead-zone handling, transmits control packets wirelessly at 250 kbps via NRF24L01+, and generates standard RC PWM servo signals (1000–2000 µs) on the receiver side. A 1-second failsafe timeout ensures safe motor cutoff on signal loss.

### System Architecture

```
TRANSMITTER                              RECEIVER
┌──────────────────┐                    ┌──────────────────┐
│  6× Analog Input │                    │  6× Servo Output │
│  (Joysticks +    │   NRF24L01+        │  (PWM 1000-2000  │
│   Potentiometers)│   250 kbps         │   µs per channel)│
│        │         │ ──────────────►    │        │         │
│  Calibration &   │   6-byte struct    │  Failsafe Timer  │
│  Dead-zone Map   │   per packet       │  (1s timeout)    │
│        │         │                    │        │         │
│  Arduino Nano    │                    │  Arduino Nano    │
└──────────────────┘                    └──────────────────┘
```

## Channel Mapping

| Channel | Function | Transmitter Pin | Receiver Pin | PWM Range |
|---------|----------|-----------------|--------------|-----------|
| 1 | Roll | A3 | D2 | 1000–2000 µs |
| 2 | Pitch | A2 | D3 | 1000–2000 µs |
| 3 | Throttle | A0 | D4 | 1000–2000 µs |
| 4 | Yaw | A1 | D5 | 1000–2000 µs |
| 5 | Aux 1 | A6 | D6 | 1000–2000 µs |
| 6 | Aux 2 | A7 | D7 | 1000–2000 µs |

## Repository Structure

```
.
├── README.md
└── firmware/
    ├── transmitter.ino    # Joystick reading, calibration, NRF24 transmission
    └── receiver.ino       # NRF24 reception, PWM generation, failsafe
```

## Hardware

- 2× Arduino Nano
- 2× NRF24L01+ radio modules (CE=9, CSN=10)
- 4× Analog joystick axes + 2× potentiometers
- 6× Servo motors (standard RC PWM)

## Dependencies

Arduino libraries: `RF24`, `Servo`, `SPI` (all available via Arduino Library Manager).

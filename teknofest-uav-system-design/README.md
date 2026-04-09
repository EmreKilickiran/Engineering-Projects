# Autonomous UAV-UGV System Design — Teknofest 9th International UAV Competition

**Team Tulpar Marmara** · Marmara University · 2024

Design report for an autonomous fixed-wing UAV system capable of deploying an unmanned ground vehicle (UGV) and payload to designated coordinates, with the UGV autonomously navigating to the payload location. Developed for the Teknofest International UAV Competition (University category).

> **Note:** This project was in the mathematical modeling and system design phase. The attached report (Turkish) documents the full aerodynamic analysis, hardware architecture, and mission design. No flight software was implemented at this stage.

## Computational & Engineering Contributions

### Aerodynamic Modeling
- Wing area calculation: 4,350 cm² (span 161.5 cm × chord 26.9 cm, AR = 6)
- Wing loading optimization: 0.66 g/cm² for stable low-speed flight
- Airfoil selection: NACA 25112, evaluated via Cl/α and Cl/Cd analysis (XFLR5)
- Multi-criteria decision analysis (MCDA) for wing geometry, tail configuration, propulsion, and material selection across 5 weighted criteria each

### Hardware Architecture & Avionics
- Pixhawk PX4 flight controller with M8N GPS, pitot tube, and 3DR 433 MHz telemetry
- Propulsion sizing: X2820-1100KV motor (880W), 70A ESC, 4S 6200mAh LiPo — thrust/weight calculations with 20–30% safety margins
- Custom Atmega328P-based control board for UGV (minimized footprint vs commercial alternatives)
- LoRa E70 433MHz star network for UAV ↔ UGV ↔ payload communication (6.5 km range)

### Mission Design
- Fully autonomous mission sequence: takeoff → waypoint navigation → low-altitude UGV deployment → payload drop with parachute → loiter until UGV confirmation → return to launch
- Custom ground control station software (developed in-house, replacing MissionPlanner)

## Repository Contents

```
.
├── README.md
└── TULPAR_MARMARA_Project_Report.pdf   # Full technical report (Turkish, 16 pages)
```

The report contains technical drawings, circuit schematics, aerodynamic plots, MCDA scoring tables, and the mission flow diagram. Visual content is interpretable regardless of language.

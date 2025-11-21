:seedling: Smart Greenhouse IoT Controller

An autonomous embedded system designed to monitor and control greenhouse climates. It features real-time sensing, closed-loop automated control for fans and irrigation pumps, and a full-stack IoT dashboard for remote monitoring and data logging.

:book: Table of Contents

Overview

System Architecture

Features

Hardware Pinout

Tech Stack

Installation & Setup

Usage Scenarios

Future Improvements

:telescope: Overview

In precision agriculture, maintaining specific temperature and humidity ranges is critical for crop yield. This project simulates a "Digital Twin" of a greenhouse controller.

The system reads environmental data from an SHT75 sensor, processes it using a PIC16F876A microcontroller, and automatically triggers:

Cooling Systems (Fan) when temperatures exceed safe limits.

Irrigation Systems (Pump) when humidity drops below thresholds.

Alarms (LED/Sounder) during critical environmental failures.

Data is transmitted via Serial (UART) to a Python server, where it is logged to CSV and visualized on a React-based dashboard.

:building_construction: System Architecture

The project bridges the simulated hardware world (Proteus) with modern software (Python/React) using a Virtual Serial Port.

graph LR
A[SHT75 Sensor] -->|Digital Data| B(PIC16F876A)
B -->|PWM/Logic| C{Actuators}
C -->|Cooling| D[Fan Relay]
C -->|Irrigation| E[Pump Relay]
B -->|UART TX/RX| F[COMPIM Model]
F -.->|Virtual Serial Cable| G[Python Flask Server]
G -->|JSON API| H[React Dashboard]
G -->|Append| I[CSV Log File]

:star: Features

:gear: Embedded Control (PIC)

Precision Sensing: Uses SHT75 for accurate Temp/Humidity readings.

Hysteresis Logic: Prevents rapid motor switching (jitter) by implementing stability zones.

Safety Mode: Triggers visual (Red LED) and auditory (Buzzer) alarms if conditions become critical.

LCD Interface: Local display of status (e.g., FAN:ON, PUMP:OFF).

:computer: IoT Software

Real-Time Dashboard: React frontend with live Chart.js graphs.

Data Logging: Automatically saves sensor history to greenhouse_logs.csv for analysis.

Live Status: Visual indicators for Fan and Pump states in the web UI.

:electric_plug: Hardware Pinout

Microcontroller: PIC16F876A (20MHz Crystal)

Component

PIC Pin

Type

Note

SHT75 Data

RC1

Input/Output

Requires Pull-up

SHT75 Clock

RC0

Output

LCD RS

RB1

Output

LCD EN

RB0

Output

LCD D4-D7

RB4-RB7

Output

4-bit Mode

Fan Relay

RA3

Output

Controls Cooling

Pump Relay

RC2

Output

Moved from RA4 (Open Drain fix)

Green LED

RA0

Output

System OK

Red LED

RA1

Output

Alarm

Buzzer

RA2

Output

Alarm

UART TX

RC6

Comms

To COMPIM TXD

UART RX

RC7

Comms

To COMPIM RXD

:hammer_and_wrench: Tech Stack

Firmware: C (XC8 Compiler, MPLAB X IDE).

Simulation: Proteus Design Suite 8 Professional.

Communication: Virtual Serial Port Driver (VSPD).

Backend: Python 3.10+, Flask, PySerial.

Frontend: HTML5, React.js (CDN), Tailwind CSS, Chart.js.

:rocket: Installation & Setup

1. Prerequisites

Proteus 8.

Python 3.x installed.

Virtual Serial Port Driver (e.g., Eltima or com0com).

2. Setup Virtual Ports

Open your VSPD software and create a pair:

Port A: COM1 (Used by Proteus)

Port B: COM2 (Used by Python)

3. Python Environment

Navigate to the project folder and install dependencies:

pip install flask flask-cors pyserial

4. Run the System

Start Server: Run python server.py. It will listen on COM2.

Open Dashboard: Go to http://localhost:5000 in your browser.

Start Simulation: Open the Proteus file, ensure COMPIM is set to COM1 (9600 baud), and press Play.

:test_tube: Usage Scenarios

Scenario

Trigger

System Response

Heatwave

Temp > 30°C

Fan turns ON, Green LED stays ON.

Cooling

Temp drops to 25°C

Fan STAYS ON (Hysteresis) until < 18°C.

Drought

Humidity < 40%

Pump turns ON to irrigate.

Emergency

Temp > 35°C

Red LED ON, Buzzer Beeps, Dashboard Alert.

:bulb: Future Improvements

[ ] Add Soil Moisture Sensor (YL-69) for better irrigation precision.

[ ] Implement Two-way communication (Control Fan from Web Dashboard).

[ ] Add ESP32 for real Wi-Fi hardware implementation.

[ ] Deploy dashboard to the cloud (Heroku/Vercel).

License

This project is open-source. Feel free to use it for educational purposes.

# 🛗 Smart Elevator Control System (Embedded C)

![Platform](https://img.shields.io/badge/Platform-C8051F020-blue)
![Language](https://img.shields.io/badge/Language-Embedded%20C-green)
![Architecture](https://img.shields.io/badge/Architecture-Event--Triggered-orange)

## 📌 Project Overview
This project is a **Smart Elevator Control System** developed for the **8051 Microcontroller (C8051F020)**. 
It features a complete logic implementation for a 5-floor building, incorporating **Safety Mechanisms** (Overload protection), **Smart Scheduling** (SCAN Algorithm), and **Real-time Sensor Monitoring**.

The system simulates a realistic elevator environment where inputs are polled via **Timer Interrupts** to ensure responsiveness, and passenger flow is tracked using IR sensors.

## ⚙️ Key Features
* **Bi-Directional Scheduling (SCAN Algorithm):** The system optimizes movement by servicing all requests in the current direction (UP) before switching to the opposite direction (DOWN), similar to modern elevator logic.
* **Safety & Overload Protection:**
    * Monitors passenger count using **Infrared (IR) Sensors**.
    * Triggers an **Overload Alarm** and prevents door closing if capacity > 4 persons.
* **Interrupt-Driven Logic:**
    * Utilizes `Timer0` Interrupts to periodically poll buttons and sensors, ensuring that user requests are never missed during motor operation.
* **Hardware Interfacing:**
    * Direct control of **H-Bridge** for Motor Direction (Up/Down).
    * **7-Segment Display** multiplexing for Floor Indication.

## 🛠️ Hardware Architecture
The code interacts directly with the MCU registers for low-level control:

| Component | Logic Function |
|-----------|----------------|
| **IR Sensors** | Passenger counting (Entry/Exit sequence detection) |
| **Timer 0** | Periodic Interrupt Generation (~240ms tick) |
| **Port 1** | 7-Segment Display Output |
| **Port 2** | Motor Control & Safety LEDs |
| **Port 5** | External Floor Call Buttons |

## 🚀 How it Works (Logic Flow)
1.  **Initialization:** Configures Crossbar, Watchdog Timer, and System Clock (2MHz).
2.  **Polling (ISR):** `Timer0` triggers every 240ms to check:
    * Floor Buttons (Internal Panel).
    * Call Buttons (External per floor).
    * Door Open Requests.
3.  **Main Loop (Scheduler):**
    * **Sweep UP:** Moves motor UP if there are requests above current floor. Checks for intermediate stops dynamically.
    * **Sweep DOWN:** Moves motor DOWN if there are requests below current floor.
4.  **Door Sequence:** * Checks IR sensors continuously while door is open.
    * Updates passenger count.
    * Checks for Overload before closing.

## 📂 File Structure

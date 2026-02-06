# MIU Firmware (Meter Interface Unit)

## 🎯 Overview

This repository contains the complete firmware for a Meter Interface Unit (MIU). The firmware is designed for STM32-based hardware and provides a comprehensive set of services for meter reading, data processing, security, and communication. It is built to be robust, with failsafe mechanisms and advanced power management for battery-operated devices.

## ✨ Key Features

*   **NB-IoT Connectivity:** Supports network communication over NB-IoT with integrated LwM2M and custom CoAP protocol support.
*   **Multi-Sensor Integration:** Interfaces with a wide range of sensors, including flow, position (accelerometer), temperature, and voltage sensors.
*   **Advanced Power Management:** Implements low-power sleep modes (STOP0, STOP2) to conserve energy, managed by a task-based scheduler.
*   **Persistent Data Logging:** Features a transactional logging system that saves device and event data to an external EEPROM.
*   **Robust Failsafe Mechanisms:** Includes software and hardware watchdogs (IWDG, WWDG), brown-out reset (BOR) handling, and power voltage detection (PVD) to ensure system reliability.
*   **Security:** Provides cryptographic services using AES and ECC for secure data handling and communication.
*   **Modular Architecture:** Organized into distinct modules for alarms, configuration, diagnosis, drivers, and more, promoting maintainability.

---

## 🗂️ Repository Structure

The firmware is organized into the following modules:

| Folder | Description |
| :--- | :--- |
| `alarm/` | Manages alarm conditions, evaluating sensor data against thresholds. |
| `config/` | Handles loading, saving, and applying device configurations from Flash memory. |
| `diagnosis/` | Implements a diagnostic logging system to record system events and errors. |
| `drivers/` | Contains low-level drivers for various hardware components like sensors, NFC tags, and fuel gauges. |
| `failsafe/` | Provides failsafe mechanisms, including watchdogs and power monitoring, to ensure system stability. |
| `ioctrl/` | Manages I/O control for power signals to peripherals like the radio and NFC chip. |
| `logger/` | A transactional logging layer that manages persistent storage of device and event logs to EEPROM. |
| `message/` | Implements a message queue and dispatcher for inter-task communication using a TLV protocol. |
| `network/` | Handles network communication, including NB-IoT, LwM2M, and custom CoAP protocols. |
| `security/` | Provides cryptographic services, including AES and ECC, for secure operations. |
| `Src/` | Contains core application source files, utilities, and configuration for peripherals like RTC and printf. |
| `system/` | Manages core system functions, including the main task scheduler, clock configuration, and power management. |

---

## ⚙️ Build Instructions

To build this project, it is recommended to use an IDE that supports STM32 development, such as **STM32CubeIDE**.

1.  **Import Project:** Import the project into your STM32CubeIDE workspace.
2.  **Configure Include Paths:** Ensure that the following top-level directories are added to the compiler's include paths:
    *   `alarm`
    *   `config`
    *   `diagnosis`
    *   `drivers`
    *   `failsafe`
    *   `ioctrl`
    *   `logger`
    *   `message`
    *   `network`
    *   `security`
    *   `Src`
    *   `system`
3.  **Build Project:** Compile the project to generate the firmware binary.

---

## 🤝 Contribution

We welcome contributions! Please follow the standard Git Flow:

1.  Fork the repository.
2.  Create a feature branch (`git checkout -b feature/NewFeature`).
3.  Commit your changes following the **Conventional Commits** standard (`feat: add new feature`).
4.  Open a Pull Request.

---

*Developed by [YK Chong/George Kent] Version 1.00 | [2025]*
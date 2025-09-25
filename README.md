# MIU Lirary 

```
Projects Folder Structure
├───.settings
├───Core
│   ├───Inc
│   ├───Src
│   └───Startup
│
├───Debug
│   ├───Core
│   │   ├───Src
│   │   └───Startup
│   └───Drivers
│
├───Drivers
│   ├───CMSIS
│   │   ├───Device
│   │   └───Include
│   └───STM32xxxx_HAL_Driver
│       ├───Inc
│       └───Src
│
├───miu_library
│   ├─── ...
│
├─PRJECT.ioc
├─PRJECT.pdf	// Project Report
├─PRJECT.txt	// Project Report
└─PRJECT_ToDoList.txt
```

```
miu_library Folder Structure
├───bootloaders
├───documents
├───drivers
│   ├───BC660K
│   ├───ENV
│   ├───ESP32
│   ├───ESP8266
│   ├───GK_TRACSENS
│   ├───M95M_EEPROM
│   ├───MAX17260_FuelGauge
│   ├───MEMS
│   │   └───LIS2DH12
│   ├───Pulser
│   ├───SIM7000E
│   ├───ST25_NFC
│   └───UBlox_LEXI_R10801D
├───examples
├───failsafe
├───network
│   ├───COAP
│   ├───GK_HES
│   ├───GK_HES_Gateway
│   ├───LWIP
│   ├───LWM2M
│   ├───NB_IoT
│   └───OpenStack
├───projects
│   ├───AURA
│   ├───MICA
│   └───TOPAZ
├───protocols
│   └───CBOR
├───RTOS
│   └───GKOS
│       ├───ALARM
│       ├───CFG
│       ├───DBG
│       ├───DIAG
│       ├───LOGGER
│       ├───MSG
│       └───SYSTEM
├───Scripts
├───security
├───storage
└───Tools
```


# 🔌 `miu_fw` (Meter Interface Unit Firmware Core)

[](https://opensource.org/licenses/MIT)
[](https://www.google.com/search?q=https://github.com/yourusername/miu_fw/releases)
[](https://www.google.com/search?q=https://github.com/yourusername/miu_fw/stargazers)

## 🎯 Overview

This repository contains the **Portable, Hardware-Agnostic Core Logic** for the Meter Interface Unit (MIU). It is designed to be integrated as a **Git Submodule** into various STM32-based projects (e.g., F4, L4, H7 variants) to provide standardized meter reading, data processing, and communication services.

The library ensures reusability by strictly separating the core business logic from the specific MCU's Hardware Abstraction Layer (HAL) using the **Platform Abstraction Layer (`port/`)**.

-----

## 🗂️ Repository Structure

The module follows a service-oriented structure:

| Folder | Description | Key Principle |
| :--- | :--- | :--- |
| **`port/`** | **Platform Abstraction Layer (The only MCU-dependent code).** Contains specific files (e.g., `stm32f4xx_port.c`) that implement the generic functions defined in `miu_port.h` using the target MCU's HAL. | **DO NOT** modify any other folder when porting to a new MCU. |
| **`modules/`** | Contains low-level drivers and fundamental services (e.g., **Delay**, **Time Management**, **Watchdog** service). | Generic, reusable logic. |
| **`network/`** | Implements standard communication protocols (e.g., **M-Bus/DLMS** stack, **TCP/IP** interface) used for data reporting. | Handles protocol encapsulation/decapsulation. |
| **`security/`** | Services for cryptographic operations, secure storage, and hardware-based security features. | Ensures data integrity and authenticity. |
| **`storage/`** | Services for managing non-volatile memory (e.g., **EEPROM Emulation**, **Flash Logging**, **Configuration Storage**). | Manages all persistent data. |
| **`examples/`** | Simple demonstration files showing how to initialize and use the core services. | Quick reference for implementation. |

-----

## ⚙️ Integration

### 1\. Adding the Submodule

From your main STM32 project directory, link this library as a submodule:

```bash
git submodule add https://github.com/yourusername/miu_fw.git MIU_Library
git submodule update --init --recursive
```

### 2\. Configuring the STM32CubeIDE Project

To successfully compile the library:

1.  **Add Include Paths:** In your project properties, add the following paths to the **GNU MCU C Compiler** → **Includes** settings:
      * `MIU_Library/port`
      * `MIU_Library/modules`
      * `MIU_Library/network`
      * *(...and all other top-level folders within `MIU_Library`)*
2.  **Select Port File:** You **must** ensure the build system only compiles the single appropriate `*_port.c` file for your target MCU (e.g., only compile `stm32f4xx_port.c` and **exclude** all others).

### 3\. Usage Example in `main.c`

After configuration, you can include and initialize the library services:

```c
#include "miu_port.h"
#include "miu_network_service.h"

int main(void)
{
    HAL_Init(); // Standard HAL initialization

    // Initialize the MIU platform abstraction
    if (MIU_Port_Init() != MIU_OK) {
        // Handle initialization error
    }

    // Start a specific MIU service
    MIU_Network_Start("192.168.1.1", 8080);
    
    while (1)
    {
        // Application logic here
    }
}
```

-----

## 🛠️ Porting to a New MCU

The strict separation of the `port/` folder makes migration straightforward:

1.  **Create a New Port File:** In the `port/` folder, duplicate an existing port file (e.g., `stm32f4xx_port.c`) and rename it for your new target (e.g., `stm32h7xx_port.c`).
2.  **Implement Functions:** Update the functions within this new file to correctly use the **HAL functions specific to the new MCU family**.
3.  **Update Build Configuration:** In your main STM32 project, ensure the build system compiles the **new** `stm32h7xx_port.c` and excludes all other port files.

## 🤝 Contribution

We welcome contributions\! Please follow the standard Git Flow:

1.  Fork the repository.
2.  Create a feature branch (`git checkout -b feature/NewProtocol`).
3.  Commit your changes following the **Conventional Commits** standard (`feat: add new protocol handler`).
4.  Open a Pull Request.

-----

*Developed by [YK Chong/Geoge Kent] Version 1.00 | [2025]*
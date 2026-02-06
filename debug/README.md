# Multi-Channel UART Debug Logger

A lightweight, non-blocking, multi-channel UART logging utility designed for embedded systems, particularly STM32 microcontrollers.

This module allows different parts of an application (e.g., communication stacks, sensor data sniffers, general debug messages) to log data concurrently over a single UART interface without interfering with each other or blocking the main application loop.

---

## 🚀 Features

* **Multi-Channel Logging**: Separates logs into distinct channels (`COM`, `SNF`, `DBG` by default) to keep debug output organized.
* **Asynchronous & Non-Blocking**: Utilizes circular buffers for each channel. Your application can quickly write log data to a buffer and continue its tasks. A dedicated `DBG_Task()` function handles the actual UART transmission in the background.
* **`printf` Redirection**: Standard `printf` calls are automatically redirected to the `DBG` channel, making it easy to integrate with existing code and standard C libraries.
* **Lightweight & Configurable**: The buffer size and channel count can be easily configured via macros in `dbg.h`.
* **Simple Parsing Protocol**: Prepends a marker like `<gLog0>` to the data stream, allowing a host-side application (e.g., a Python script or terminal) to easily parse and color-code the output from different channels.

---

## 🔧 How It Works

The logger uses a circular buffer for each defined channel. When your application calls a logging function like `DBG_Channel_Printf()` or `printf()`, the data is written to the tail of the corresponding buffer.

The `DBG_Task()` function, which should be called continuously from the main `while(1)` loop, acts as a simple scheduler. It checks if the UART peripheral is ready to transmit a new byte. If it is, it pulls the next available byte from one of the channels, sends it, and moves on. The task cycles through the channels to ensure all pending data is eventually sent.

---

## 🛠️ Getting Started

Follow these steps to integrate the debug logger into your project.

### 1. Add Files to Project

* Copy `dbg.c` and `dbg.h` into your project's source and include directories.
* Ensure the files are included in your build system (e.g., Makefile or IDE project settings).

### 2. Initialization

In your `main.c` or initialization code, include the header and call `DBG_Init()` after your UART peripheral has been configured.

```c
#include "app_main.h"

void APP_Init(void)
{
    DBG_Init(huart1.Instance);
    DBG_Channel_Printf(CHANNEL_DBG,"Debug module initialized!\n");
    DBG_Channel_Printf(CHANNEL_SNF,"From SNF\n");
    DBG_Channel_Printf(CHANNEL_COM,"From COM\n");
}
```

### 3. Main Loop Integration

Call `DBG_Task()` continuously within your main `while(1)` loop. This function is non-blocking and will handle the transmission of buffered log data.

```c
void APP_Main(void)
{
    DBG_Task();
}
```

### 4. Logging Messages

Use the `DBG_Channel_Printf()` function to send formatted strings to a specific channel.

```c
// Add this enum definition to dbg.h for clarity
typedef enum {
    CHANNEL_COM = 0,
    CHANNEL_SNF = 1,
    CHANNEL_DBG = 2
} DBG_Channel_t;
```

```c
<gLog2>:
Debug module initialized!

<gLog0>:
From COM

<gLog1>:
From SNF
```

---

## 📋 API Reference

#### `void DBG_Init(USART_TypeDef* uart_instance);`
Initializes the debug module with a specific UART peripheral instance.

#### `void DBG_Task(void);`
The main task function that services the transmit buffers. **Must be called repeatedly from the main loop.**

#### `void DBG_Channel_Printf(DBG_Channel_t channel, const char *format, ...);`
Prints a formatted string to the specified channel's buffer. This is the recommended function for most logging.

#### `void DBG_Print(f_, ...)` & `void DBG_PrintLine(f_, ...)`
Macros that wrap `printf` to send data to the default `DBG` channel. `DBG_PrintLine` adds a newline.

#### `void DBG_COM_PutByte(uint8_t _byte);`
`void DBG_SNF_PutByte(uint8_t _byte);`
`void DBG_DBG_PutByte(uint8_t _byte);`
Low-level functions to add a single byte to a specific channel's buffer.

---

## ⚙️ Configuration

The following settings can be adjusted in `dbg.h`:

* `#define DBG_CFG_BUF_SIZE 512`
    * Controls the size (in bytes) of the circular buffer for **each** channel. Increase this if you log large bursts of data.

* `#define DBG_CFG_NO_OF_CHANNEL 3`
    * Defines the number of logging channels available.

---

##  Dependencies

* This module is written using the **STM32 Low-Layer (LL) libraries**.
* It can be easily adapted to use HAL libraries or other platforms by modifying the `LL_USART_...` calls in `DBG_Init()` and `DBG_Task()`.
# FreeRTOS - Project Sensor LED WiFi (ESP + Arduino)

This project demonstrates a modular firmware architecture for the ESP32, combining FreeRTOS task management with Arduino hardware APIs. Each module runs as an independent FreeRTOS task with its own initialization, logic, and cleanup routines.

## Key Features

* Modular design with independent FreeRTOS tasks
* Uses Arduino framework for hardware abstraction
* Structured initialization and cleanup pattern
* Demonstrates practical RTOS task prioritization
* Easily extendable for additional modules (e.g., MQTT, sensor fusion, motor control)

## Getting Started

1. Install PlatformIO (VSCode extension recommended).
2. Create a new project with the above configuration.
3. Ensure you have the Arduino-ESP32 framework installed (PlatformIO will handle this automatically).
4. Build and upload the project to your ESP Board.

## System Overview

The firmware consists of three main modules, each implemented as a FreeRTOS task. They run concurrently and communicate through shared state and periodic checks.

| Module | Description | Task Priority |
|--------|-------------|---------------|
| Sensor Reader | Periodically generates or reads sensor data (temperature, humidity, voltage). | 6 |
| LED Controller | Updates LED patterns based on WiFi state. | 5 |
| WiFi Manager | Handles WiFi connection lifecycle and state tracking. | 4 |

The main loop periodically prints system status and manages LED updates according to WiFi state.

## Project Structure
```
src/
 ├── main.cpp
 └── modules/
      ├── led_controller/
      │   ├── led_controller.h
      │   └── led_controller.cpp
      ├── wifi_manager/
      │   ├── wifi_manager.h
      │   └── wifi_manager.cpp
      └── sensor_reader/
          ├── sensor_reader.h
          └── sensor_reader.cpp
```

Each module provides:

* A `*_create()` and `*_destroy()` function for initialization and cleanup.
* A `*_start()` function that creates a dedicated FreeRTOS task.
* Accessor and control functions for inter-module coordination.

## Operation Summary

### 1. Initialization (setup)

* Initializes serial logging.
* Creates and starts all module tasks.
* Sets initial LED blink pattern.

### 2. Loop Execution

* Runs every 10 seconds.
* Logs:
  * Sensor data
  * WiFi connection state
  * FreeRTOS heap usage
* Updates LED patterns according to WiFi status.
* Uses small delays (`vTaskDelay`) to avoid watchdog resets.

### 3. Cleanup

* Destroys all module instances (not typically reached in normal operation).

## Example Log Output
```
[I][Main]: ESP32-S3 RTOS Example Starting...
[I][Main]: All modules initialized and tasks started
[I][SensorReader]: Temp: 25.3C, Hum: 55.1%, Volt: 2.10V
[I][WiFiManager]: Connecting to WiFi...
[I][LedController]: Pattern set: BLINK_FAST
[I][Main]: === System Status ===
[I][Main]: WiFi - State: Connected
[I][Main]: Free Heap: 264312 bytes
[I][Main]: Minimum Heap Free: 250120 bytes
```
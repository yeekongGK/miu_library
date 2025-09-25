## SYSTEM Folder
1. Main Purpose:
    * Core system management and initialization
    * Task scheduling and state management
    * System timing and tick management
    * Power management (sleep/wake functionality)

2. Key Modules:
    * `sys.h` - System interfaces and definitions
    * `sys.c` - Core system implementation
    * Task state management structures
    * Timer management

3. Interactions:
    ```
    graph TD
        SYS[SYSTEM] --> ALARM[Alarm Module]
        SYS --> LOGGER[Logger]
        SYS --> TASK[Task Management]
        SYS --> POWER[Power Management]
    ```
4. Patterns/Conventions:
    * Task states defined as enums (SLEEP_TaskState, RUN_TaskState)
    * Standard initialization pattern SYS_Init()
    * Consistent timing functions (SYS_GetTick_ms())
    * Task ID based management
    * Power state management functions
5. Example Usage:
    ```
    // System timing example
        uint32_t currentTime = SYS_GetTick_ms();

        // Task state management
        SYS_Sleep(TASK_ID, sleep_period);
        if(SYS_IsAwake(TASK_ID)) {
            // Task processing
        }
    ```

## LOGGER Folder
1. Main Purpose:
    * Data logging functionality
    * Event recording
    * System history maintenance
    * Log file management
2. Key Modules:
    * `logger.h` - Logger definitions
    * `logger.c` - Logger implementation
    * Log entry structures
    * Storage management
3. Interactions:
    ```
    graph TD
        LOGGER[LOGGER] --> SYSTEM[System Module]
        LOGGER --> STORAGE[Storage]
        LOGGER --> ALARM[Alarm Events]
        LOGGER --> TIME[Timestamp]
    ```
4. Patterns/Conventions:

    * Circular buffer implementation
    * Timestamp-based entries
    * Priority levels for logs
    * Standard log format
5. Example Usage:
    ```
    // Log event with timestamp
    LOGGER_LogEvent(EVENT_TYPE, data, sizeof(data));

    // Query log history
    LOGGER_GetEntries(buffer, count);
    ```

## ALARM Folder
1. Main Purpose:
    * Manages system alarms and thresholds
    * Handles alarm conditions and triggers
    * Provides debouncing and timing control
    * Maintains alarm states
2. Key Modules: From the visible code:
    * alarm.h - Alarm definitions
    * alarm.c - Core alarm implementation
    * Alarm object structures
    * Threshold management
3. Interactions:
    ```
    graph TD
        ALARM[ALARM] --> SENSOR[Sensor Module]
        ALARM --> SYSTEM[System Module]
        ALARM --> TLV[TLV Protocol]
        ALARM --> DBG[Debug Module]
    ```
4. Patterns/Conventions: From alarm.c:
    * State machine implementation (IDLE_AlarmObjectState, ARMED_AlarmObjectState, etc.)
    * TLV-based configuration
    * Consistent threshold comparison patterns
    * Object-based alarm management
    * Clear separation of concerns (comparison, state management, configuration)

5. Example Usage:
    ```
    // Configure alarm threshold
    ALARM_AlarmObject_t alarm = {
        .enabled = true,
        .sensor = SENSOR_1,
        .threshold1Type = ABOVE_AlarmThreshold,
        .threshold1 = 50.0f,
        .debouncePeriod_ms = 1000
    };
    // Check alarm status
    uint32_t activeAlarms = ALARM_GetActiveBitmap();

    // Process alarm in task
    ALARM_Task();
    ```

##### Common Patterns Across All Three:

1. Task-based architecture with clear state management
2. Consistent use of configuration structures
3. Power-aware design with sleep/wake cycles
4. TLV-based configuration interface
5. Standard initialization patterns
6. Clear error handling and status reporting

##### The code shows a well-structured embedded system with careful attention to:
* Resource management
* Power efficiency
* Clear state transitions
* Configurable behavior
* Robust error handling
* Modular design principles
# GitHub Copilot Instructions for ESP32 CalDAV Calendar Firmware

## Project Overview

This is an ESP32-S3 based e-paper calendar display firmware using the ESP-IDF framework. The device periodically wakes from deep sleep, connects to a WiFi network, fetches calendar events from a CalDAV server, renders them on an e-paper display via LVGL, and returns to deep sleep.

**Key Technologies:**
- Platform: ESP32-S3 (ESP-IDF framework)
- Build System: PlatformIO
- RTOS: FreeRTOS
- GUI: LVGL (rendered to e-paper via FastEPD)
- Storage: NVS, LittleFS (16 MB flash)
- Networking: WiFi (STA), CalDAV client, SNTP

---

## Code Style and Formatting

### General Formatting Rules

The project uses **Artistic Style (AStyle)** with a K&R-based configuration (`scripts/astyle.cfg`):

- **Style**: K&R (Kernighan & Ritchie)
- **Indentation**: 4 spaces (no tabs)
- **Line Length**: Maximum 120 characters
- **Braces**: K&R style (opening brace on same line for functions, control structures)
- **Operators**: Space padding around operators: `a = bar((b - c) * a, d--);`
- **Headers**: Space between header and bracket: `if (condition) {`
- **Pointers/References**: Stick to name: `char *pThing`, `char &thing`
- **Conditionals**: Always use braces, even for single-line blocks
- **Switch**: Indent case statements

**Example:**
```cpp
esp_err_t MyFunction(uint8_t *p_Buffer, size_t Size)
{
    if (p_Buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (size_t i = 0; i < Size; i++) {
        p_Buffer[i] = ProcessByte(p_Buffer[i]);
    }
    
    return ESP_OK;
}
```

### Type Casting

**Always use C++ style casts** instead of C-style casts. C++ casts are more explicit, safer, and easier to search for in code.

#### Cast Types

- **`static_cast<T>()`** - Use for safe, well-defined conversions:
  - Casting between numeric types: `static_cast<uint8_t>(value)`
  - Casting `void*` from `malloc()` to typed pointer: `static_cast<MyType *>(malloc(...))`
  - Upcasting in class hierarchies (when applicable)
  - Explicit conversions where implicit conversion would work
  
  **Example:**
  ```cpp
  Event_Node_t *p_Node = static_cast<Event_Node_t *>(malloc(sizeof(Event_Node_t)));
  uint8_t value = static_cast<uint8_t>(largerValue & 0xFF);
  ```

- **`reinterpret_cast<T>()`** - Use for low-level, potentially unsafe conversions:
  - Converting between unrelated pointer types: `uint8_t*` ↔ `char*`
  - Casting pointers to integers or vice versa
  - Reinterpreting binary data representations
  - Hardware register access or memory-mapped I/O
  
  **Example:**
  ```cpp
  strcpy(reinterpret_cast<char *>(WiFiConfig.sta.ssid), SSID);
  uint16_t *p_Data = reinterpret_cast<uint16_t *>(&buffer[offset]);
  ```

- **`const_cast<T>()`** - Use to add or remove `const` qualifier:
  - Only when absolutely necessary (e.g., interfacing with C APIs that don't use `const`)
  - Prefer refactoring to avoid when possible
  
  **Example:**
  ```cpp
  /* Only when interfacing with API that should accept const but doesn't */
  legacy_api(const_cast<char *>(constString));
  ```

- **`dynamic_cast<T>()`** - Use for runtime type checking in polymorphic hierarchies:
  - Requires RTTI (Runtime Type Information)
  - Not commonly used in embedded ESP-IDF projects due to overhead
  - Only applicable with virtual functions and inheritance
  
  **Note:** This project typically avoids `dynamic_cast` due to embedded constraints.

#### Migration from C-Style Casts

❌ **Don't use C-style casts:**
```cpp
p_Node = (Event_Node_t *)malloc(sizeof(Event_Node_t));           // Bad
s16 = (uint16_t *)&px_map[offset];                               // Bad
return (*(int *)a - *(int *)b);                                  // Bad
```

✅ **Use C++ style casts:**
```cpp
p_Node = static_cast<Event_Node_t *>(malloc(sizeof(Event_Node_t)));        // Good
s16 = reinterpret_cast<uint16_t *>(&px_map[offset]);                       // Good
return (*static_cast<const int *>(a) - *static_cast<const int *>(b));      // Good
```

#### Guidelines

- **Search for casts:** C++ casts are easier to find with regex: `grep "static_cast\|reinterpret_cast"`
- **Document unsafe casts:** Add comments explaining why `reinterpret_cast` is needed
- **Minimize casting:** Design APIs to reduce the need for casts when possible
- **Const correctness:** Use proper const qualifiers to avoid needing `const_cast`

### Naming Conventions

#### Functions
- **Public API**: `ModuleName_FunctionName()` using PascalCase
  - Examples: `SettingsManager_Init()`, `NetworkManager_Connect()`, `TimeManager_SetTimezone()`
- **Private/Static**: `snake_case` with descriptive names
  - Examples: `on_WiFi_Event()`, `run_astyle()`

#### Variables
- **Local variables**: `PascalCase` for important variables, `lowercase` for simple counters
  - Examples: `RetryCount`, `EventGroup`, `Error`, `i`, `x`
- **Pointers**: Prefix with `p` (e.g., `p_Settings`, `p_Buffer`, `p_Data`)
- **Global/Static module state**: Prefix with underscore: `_State`, `_Network_Manager_State`, `_App_Context`

#### Constants and Macros
- **All UPPERCASE** with underscores: `WIFI_CONNECTED_BIT`, `NVS_NAMESPACE`, `SETTINGS_VERSION`
- Module-specific macros should include module name: `SETTINGS_NVS_NAMESPACE`, `CALDAV_DEFAULT_TIMEOUT`

#### Types
- **Structs/Enums**: `ModuleName_Description_t` with `_t` suffix
  - Examples: `App_Settings_t`, `Network_State_t`, `App_Settings_WiFi_t`
- **Enums**: Use descriptive prefix for values
  - Example: `SETTINGS_EVENT_LOADED`, `NETWORK_EVENT_WIFI_CONNECTED`

#### File Names
- Header files: `moduleName.h`
- Implementation files: `moduleName.cpp`
- Types/definitions: `moduleTypes.h`
- Private implementations: `Private/internalModule.cpp`

### Code Organization

#### Directory Structure
```
main/
├── main.cpp                   # Application entry point
├── Devices/                   # Low-level peripheral drivers
│   ├── devices.h              # Aggregated device includes
│   ├── ADC/                   # Battery voltage ADC driver
│   ├── I2C/                   # I2C bus initialisation
│   └── RTC/                   # BM8563 RTC driver
├── EventManager/              # ESP Event bus helpers
├── Export/fonts/              # Custom LVGL font files
├── Settings/                  # Settings manager (NVS + JSON loader)
│   ├── settingsManager.h      # Public API
│   ├── settingsManager.cpp    # Implementation
│   ├── settingsTypes.h        # Public types (App_Settings_t)
│   └── Private/               # Internal loaders
│       ├── settingsLoader.h
│       ├── settingsJSONLoader.cpp
│       └── settingsDefaultLoader.cpp
├── SNTP/                      # NTP synchronisation helper
├── TimeManager/               # Time management and RTC abstraction
├── UI/                        # LVGL screen definitions
│   ├── ui.h                   # Shared UI declarations
│   ├── eventView.h/.cpp       # Event view widget
│   ├── ui_dashboard.cpp       # Monthly calendar view
│   ├── ui_dayview.cpp         # Agenda / list view
│   └── ui_status.cpp          # Status bar
└── WiFi/                      # WiFi driver and connection management
```

#### Private Implementations
Use `Private/` subdirectories for internal module implementations that should not be exposed:
```
Settings/
├── settingsManager.h         # Public API
├── settingsManager.cpp       # Implementation
├── settingsTypes.h           # Public types
└── Private/
    ├── settingsLoader.h      # Internal interface
    ├── settingsJSONLoader.cpp
    └── settingsDefaultLoader.cpp
```

---

## License and Copyright

### File Headers

**Every source file** (`.cpp`, `.h`, `.py`) must include the following header:

```cpp
/*
 * filename.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Brief description of the file's purpose.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Errors and commissions should be reported to DanielKampert@kampis-elektroecke.de
 */
```

For Python files:
```python
"""
filename.py

Copyright (C) Daniel Kampert, 2026
Website: www.kampis-elektroecke.de
File info: Brief description of the file's purpose.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
"""
```

**License**: GNU General Public License v3.0  
**Copyright Holder**: Daniel Kampert  
**Contact**: DanielKampert@kampis-elektroecke.de  
**Website**: www.kampis-elektroecke.de

---

## Documentation Standards

### Code Comments

#### Doxygen-Style Documentation
Use Doxygen comments for all public API functions:

```cpp
/** @brief          Brief description of the function.
 *  @param p_Param  Description of parameter
 *  @param Size     Description of size parameter
 *  @return         ESP_OK on success, ESP_ERR_* on failure
 */
esp_err_t MyFunction(uint8_t *p_Param, size_t Size);
```

#### Inline Comments
- Use `//` for single-line comments
- Use `/* */` for block comments
- Add explanatory comments for complex logic
- Comment "why", not "what" (the code shows what)

**Example:**
```cpp
/* Reset config_loaded flag to allow reloading default config */
Error = nvs_set_u8(_State.NVS_Handle, "config_loaded", false);
```

### Structure Documentation

Document struct members inline:

```cpp
typedef struct {
    char SSID[33];              /**< WiFi SSID. */
    char Password[65];          /**< WiFi password. */
    uint8_t MaxRetries;         /**< Maximum number of connection retries. */
    uint32_t TimeoutMs;         /**< Connection timeout in milliseconds. */
} App_Settings_WiFi_t;
```

### Enum Documentation

```cpp
/** @brief Settings event identifiers.
 */
enum {
    SETTINGS_EVENT_LOADED,      /**< Settings loaded from NVS. */
    SETTINGS_EVENT_SAVED,       /**< Settings saved to NVS. */
    SETTINGS_EVENT_WIFI_CHANGED,/**< WiFi settings changed.
                                     Data contains App_Settings_WiFi_t. */
};
```

### External Documentation

For complex modules, create documentation in `docs/` directory:
- Use **AsciiDoc** (`.adoc`) for technical documentation
- Use **Markdown** (`.md`) for README-style documentation
- Include architecture diagrams, API references, usage examples, and troubleshooting

---

## ESP-IDF Specific Conventions

### Error Handling

- **Always check return values** from ESP-IDF functions
- Use `ESP_ERROR_CHECK()` for critical initialization that should abort on failure
- Use manual error handling for recoverable errors:

```cpp
esp_err_t Error = nvs_open(NAMESPACE, NVS_READWRITE, &Handle);
if (Error != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS: %d", Error);
    return Error;
}
```

### Logging

Use ESP-IDF logging macros with appropriate levels:
- `ESP_LOGE(TAG, ...)` - Errors
- `ESP_LOGW(TAG, ...)` - Warnings
- `ESP_LOGI(TAG, ...)` - Information
- `ESP_LOGD(TAG, ...)` - Debug

Define TAG at the top of each file:
```cpp
static const char *TAG = "module_name";
```

### Event System

- Use ESP Event system for module communication
- Define event bases: `ESP_EVENT_DEFINE_BASE(MODULE_EVENTS);`
- Declare in headers: `ESP_EVENT_DECLARE_BASE(MODULE_EVENTS);`
- Use descriptive event IDs in enums
- Always include documentation about event data payload

### FreeRTOS

- Task names should be descriptive: `"DevicesTask"`, `"NetworkTask"`
- Use appropriate priorities (defined in task headers)
- Always check if queue/semaphore creation succeeded
- Use `portMAX_DELAY` for blocking operations unless timeout is critical
- Prefer queues for inter-task communication

---

## Module Design Patterns

### Manager Pattern

Managers are stateful modules that provide a cohesive API for a subsystem:

```cpp
// Public API pattern
esp_err_t ModuleName_Init(void);
esp_err_t ModuleName_Deinit(void);
esp_err_t ModuleName_GetConfig(Module_Config_t *p_Config);
esp_err_t ModuleName_UpdateConfig(Module_Config_t *p_Config);
esp_err_t ModuleName_Save(void);

// Internal state (static in .cpp)
static Module_State_t _State;
```

### Thread-Safe Access Pattern

For shared resources:

```cpp
typedef struct {
    SemaphoreHandle_t Mutex;
    // ... data fields
} Module_State_t;

esp_err_t Module_GetData(Data_t *p_Data)
{
    if (p_Data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xSemaphoreTake(_State.Mutex, portMAX_DELAY);
    memcpy(p_Data, &_State.Data, sizeof(Data_t));
    xSemaphoreGive(_State.Mutex);
    
    return ESP_OK;
}
```

### Event-Driven Updates

When updating module state:
1. Validate parameters
2. Acquire mutex
3. Update state
4. Release mutex
5. Post event to notify listeners

```cpp
esp_err_t Module_Update(Config_t *p_Config)
{
    if (_State.isInitialized == false) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xSemaphoreTake(_State.Mutex, portMAX_DELAY);
    memcpy(&_State.Config, p_Config, sizeof(Config_t));
    xSemaphoreGive(_State.Mutex);
    
    esp_event_post(MODULE_EVENTS, MODULE_EVENT_CONFIG_CHANGED, 
                   p_Config, sizeof(Config_t), portMAX_DELAY);
    
    return ESP_OK;
}
```

---

## Settings Management

### Settings Structure

- Settings are organized into three categories: WiFi, System, CalDAV
- Each category has a dedicated struct type: `App_Settings_WiFi_t`, `App_Settings_System_t`, `App_Settings_CalDAV_t`
- The complete settings are held in `App_Settings_t` which also includes a `std::list<std::string>` of calendar names
- Use `__attribute__((packed))` for settings structures stored in NVS
- Include reserved fields for future expansion

### Settings API Pattern

Each settings category follows this pattern:

```cpp
esp_err_t SettingsManager_GetWiFi(App_Settings_WiFi_t* p_Settings);
esp_err_t SettingsManager_GetSystem(App_Settings_System_t* p_Settings);
esp_err_t SettingsManager_GetCalDAV(App_Settings_CalDAV_t* p_Settings);
void SettingsManager_GetCalendars(std::list<std::string> *p_Calendars);
```

**Important**: These functions read from RAM only. Call `SettingsManager_Save()` to persist changes!

### Factory Defaults

Two-tier default system:
1. **JSON Config** (`data/settings_default.json`) - Preferred, loaded on first boot from LittleFS
2. **Hardcoded Defaults** - Fallback in code

---

## Network and Communication

### WiFi Management

- Station (STA) mode only — AP mode is not used
- Implement retry logic with configurable max attempts (`MaxRetries`)
- Connection timeout configurable per attempt (`TimeoutMs`)
- Use ESP Event system for connection state changes
- Store credentials securely in NVS via Settings Manager

### CalDAV Client

- Uses the `ESP32-CalDAV` component (`components/ESP32-CalDAV/`)
- Configured with server URL, username, password and request timeout
- Fetches events from one or more named calendars specified in settings
- Called once per wake cycle; results are rendered to the display before deep sleep

---

## Device Integration

### I2C Devices

- I2C bus initialised once in `Devices/I2C/`
- **BM8563 RTC** (`Devices/RTC/`) – keeps time across deep sleep cycles; read on wake to restore `struct tm`
- **GT911 touch controller** – capacitive touch input for the e-paper panel, used via `esp_lcd_touch_gt911`
- Create device handles for each peripheral, implement proper error handling and recovery

### E-Paper Display

- Driven by the **FastEPD** library (`components/FastEPD/`)
- LVGL renders into a draw buffer which is flushed to FastEPD via `epaper_display_flush()`
- Full refresh vs. partial update managed by counting LVGL flush calls
- Draw buffer allocated in PSRAM for large frame sizes

### Battery ADC

- Battery voltage measured via ADC (`Devices/ADC/`) through a resistor voltage divider
- Configurable via Kconfig: ADC unit, channel, attenuation, R1/R2 values, sample count, EMA alpha, outlier threshold

---

## Build and Tooling

### PlatformIO Configuration

- Default environment: `debug`
- Board: `esp32-s3-devkitm-1`
- Flash size: 16 MB
- PSRAM: OPI mode
- Filesystem: LittleFS

### Pre/Post Build Scripts

- `scripts/clean.py` - Clean build artifacts (pre-build)
- `scripts/format.py` - Format code with AStyle (post-build)

### Formatting

Run formatting manually:
```bash
astyle --options=scripts/astyle.cfg "main/**/*.cpp" "main/**/*.h"
```

---

## Documentation Maintenance

### AsciiDoc Documentation in `docs/`

The project maintains comprehensive AsciiDoc documentation in the `docs/` directory. **When modifying code, always update the corresponding documentation.**

#### Documentation Structure

```
docs/
├── index.adoc              # Overview and getting started
├── SettingsManager.adoc    # Settings management system
├── WiFi.adoc               # WiFi driver and connection management
├── CalDAV.adoc             # CalDAV client integration
├── TimeManager.adoc        # Time management and RTC
├── SNTP.adoc               # NTP synchronisation
├── Devices.adoc            # Device drivers (ADC, I2C, RTC)
└── UI.adoc                 # LVGL UI views and event rendering
```

#### When to Update Documentation

**Always update the corresponding `.adoc` file when:**
- Adding new public API functions
- Modifying function signatures or parameters
- Changing behavior or semantics of existing functions
- Adding new modules or managers
- Modifying settings structure or event types
- Changing initialization requirements or dependencies
- Adding new features or capabilities
- Fixing bugs that affect documented behavior

#### Documentation Update Pattern

1. **Update Code First**: Make your code changes
2. **Update Documentation**: Modify the relevant `.adoc` file(s)
3. **Keep in Sync**: Ensure examples, function signatures, and descriptions match the code exactly
4. **Test Documentation**: Verify that code examples in documentation still compile and work

#### Module-to-Documentation Mapping

| Code Location | Documentation File |
|--------------|-------------------|
| `main/Settings/` | `SettingsManager.adoc` |
| `main/WiFi/` | `WiFi.adoc` |
| `components/ESP32-CalDAV/` | `CalDAV.adoc` |
| `main/TimeManager/` | `TimeManager.adoc` |
| `main/SNTP/` | `SNTP.adoc` |
| `main/Devices/` | `Devices.adoc` |
| `main/UI/` | `UI.adoc` |

#### Documentation Style Guidelines

- Use AsciiDoc syntax consistently
- Include code examples for new API functions
- Document all parameters, return values, and error codes
- Add diagrams for complex workflows (using PlantUML or similar)
- Keep examples realistic and tested
- Document thread-safety and RTOS considerations
- Include usage warnings and common pitfalls

**Example Documentation Entry:**
```asciidoc
=== MyModule_DoSomething()

[source,c]
----
esp_err_t MyModule_DoSomething(uint8_t *p_Data, size_t Size);
----

Performs an important operation on the provided data buffer.

**Parameters:**

* `p_Data` - Pointer to data buffer (must not be NULL)
* `Size` - Size of data buffer in bytes

**Return Values:**

* `ESP_OK` - Operation successful
* `ESP_ERR_INVALID_ARG` - NULL pointer or invalid size
* `ESP_ERR_INVALID_STATE` - Module not initialized

**Thread Safety:** This function is thread-safe and can be called from multiple tasks.

**Example:**
[source,c]
----
uint8_t Buffer[128];
esp_err_t Error = MyModule_DoSomething(Buffer, sizeof(Buffer));
if (Error != ESP_OK) {
    ESP_LOGE(TAG, "Operation failed: %d", Error);
}
----
```

#### Automated Documentation Build

Documentation is automatically built and deployed via GitHub Actions when a workflow for documentation is configured. The CI/CD pipeline should:
- Build all `.adoc` files to HTML and PDF
- Deploy to GitHub Pages: https://kampi.github.io/ESP32-Calendar/
- Create release artifacts with PDF documentation

**Do not commit generated HTML/PDF files** - these are built automatically by the CI/CD pipeline.

---

## Testing and Debugging

### Debugging

- Use `ESP_LOGD` for debug output (disabled in release builds)
- Enable debug build type for verbose logging: `build_type = debug`
- Use ESP-IDF monitor with exception decoder: `monitor_filters = esp32_exception_decoder`

### Error Reporting

- Log errors with context: function name, error code, relevant parameters
- Include error strings when available
- Use descriptive error messages

---

## Best Practices

### Memory Management

- Check malloc/calloc/queue/semaphore creation success
- Free resources in deinit functions
- Use PSRAM for large buffers (frame buffers, JSON parsing)
- Be mindful of stack sizes for tasks

### Initialization Order

1. Event loop
2. NVS Flash
3. Settings Manager (loads from NVS)
4. Devices (I2C bus, BM8563 RTC, GT911 touch, ADC)
5. Time Manager (reads RTC to restore current time)
6. WiFi (connect, sync NTP if needed)
7. CalDAV client (fetch events)
8. GUI (render events to e-paper display)
9. Deep sleep

### Configuration

- All configurable parameters should go through Settings Manager
- Avoid hardcoded values that users might want to change
- Provide sensible defaults
- Document valid ranges and constraints

### Maintainability

- Keep functions focused and small
- Extract complex logic into separate functions
- Use descriptive variable names
- Document assumptions and constraints
- Write self-documenting code where possible

---

## Common Pitfalls to Avoid

❌ **Don't:**
- Mix tabs and spaces
- Exceed 120 character line length
- Forget error checking on ESP-IDF calls
- Access shared state without mutex protection
- Forget to call `SettingsManager_Save()` after updates
- Use blocking operations in ISRs
- Ignore compiler warnings
- Commit code without verifying it compiles
- Use C-style casts instead of C++ casts

✅ **Do:**
- Use consistent naming conventions
- Document all public APIs
- Include license headers in all files
- Test error paths
- Use appropriate log levels
- Clean up resources on failure
- Follow the established module patterns
- Use C++ style casts (static_cast, reinterpret_cast, const_cast) instead of C-style casts
- **Update corresponding AsciiDoc documentation when changing code**
- **Verify code compiles and links before committing**

---

## Code Verification Process

### Pre-Commit Checks

Before committing code changes, **always verify**:

1. **Compilation**: Code must compile without errors
   ```bash
   platformio run --environment debug
   ```

2. **Syntax Errors**: Check for syntax issues
   - Review compiler output for errors and warnings
   - Address all `-Werror` warnings (warnings treated as errors)

3. **Linker Errors**: Ensure all symbols are resolved
   - Missing includes: Add required headers
   - Undefined references: Check function declarations/definitions
   - Multiple definitions: Remove duplicate symbols

4. **Common Issues to Check**:
   - Missing `#include` statements for used functions
   - Mismatched function signatures between declaration and definition
   - Missing library dependencies in `platformio.ini`
   - Incorrect use of ESP-IDF APIs (check version compatibility)

5. **Runtime Verification** (when possible):
   - Upload and test on hardware
   - Check for runtime errors in serial monitor
   - Verify expected behavior

### Build System Integration

The project uses PlatformIO with automatic formatting:
- Pre-build: `scripts/clean.py` cleans build artifacts
- Post-build: `scripts/format.py` formats code with AStyle

**Always run a full clean build before final commit:**
```bash
platformio run --target clean
platformio run --environment debug
```

---

## Additional Resources

- **ESP-IDF Documentation**: https://docs.espressif.com/projects/esp-idf/
- **FreeRTOS Documentation**: https://www.freertos.org/
- **LVGL Documentation**: https://docs.lvgl.io/
- **FastEPD Library**: https://github.com/bitbank2/FastEPD
- **CalDAV RFC 4791**: https://www.rfc-editor.org/rfc/rfc4791
- **POSIX Timezone Database**: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
- **Project Repository**: https://github.com/Kampi/ESP32-Calendar

---

**Last Updated**: April 27, 2026
**Maintainer**: Daniel Kampert (DanielKampert@kampis-elektroecke.de)

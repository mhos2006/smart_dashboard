#  ESP32 Smart Dashboard & Meteorological Hub

A completely over-engineered, web-controlled smart dashboard built on an ESP32 and a 2.8-inch ILI9341 SPI TFT display. Originally intended to use physical tactile buttons, which were promptly thrown in the bin because plastic breadboards and 1mm component legs are a miserable combination. 

This project serves as a live weather station for Manchester, UK, and doubles as a Wi-Fi-controlled Pomodoro and gym timer. It is the rare first-year EEE project that actually survived long enough to become a permanent desk fixture without catching fire.

## Core Features

*   **Live Meteorological Telemetry:** Integrates with the OpenWeatherMap API via JSON parsing to display real-time temperature, weather conditions, humidity, wind speed (converted to mph), and sunrise/sunset times.
*   **Atomic Timekeeping:** Synchronizes with `pool.ntp.org` to provide accurate time and date, automatically adjusting for British Summer Time offsets.
*   **Remote Web Server Control:** Hosts an asynchronous HTML/CSS web interface on Port 80. Accessible via any mobile browser on the local network to trigger dual-mode timers.
*   **Screen Hijacking UI:** Features a dynamic interface that temporarily wipes the atomic clock to display a massive stopwatch or countdown timer when triggered by the web app, complete with anti-ghosting pixel redraws.

## Hardware Requirements

*   ESP32 Development Board
*   2.8" ILI9341 SPI TFT LCD Display
*   A reliable 5V wall charger (laptop USB ports will struggle with the Wi-Fi current spikes)
*   A concerning amount of DuPont jumper wires

**Wiring Configuration (SPI Bus):**
*   **MOSI:** 23
*   **MISO:** 19
*   **SCK:** 18
*   **CS:** 15
*   **DC:** 2
*   **RST:** 4
*   **VCC/LED:** 3.3V or 5V (depending on board tolerance)
*   **GND:** GND

## Software Dependencies

This project relies on a strict `#include` hierarchy to prevent namespace collisions between the web server and the screen drivers. Ensure the libraries are loaded in this exact order:

1.  `WiFi.h`
2.  `FS.h` (Must precede WebServer to prevent compiler errors)
3.  `WebServer.h`
4.  `HTTPClient.h`
5.  `time.h`
6.  `SPI.h`
7.  `TFT_eSPI.h`
8.  `ArduinoJson.h`

## Installation & Setup

1.  **Configure the TFT Library:** Navigate to your Arduino libraries folder, open `TFT_eSPI/User_Setup.h`, and uncomment the ILI9341 driver and the specific ESP32 hardware pins listed above.
2.  **Update Credentials:** Open the main sketch and insert your local Wi-Fi SSID, network password, and OpenWeatherMap API key.
3.  **Flash the Board:** Connect the ESP32 to your laptop and upload the code.
4.  **Acquire the IP:** Open the Arduino Serial Monitor at 115200 baud to retrieve the local IP address assigned by your router.
5.  **Deploy:** Unplug the ESP32 from your computer, plug it into a dedicated wall charger, and type the IP address into your phone's browser to access the control panel.

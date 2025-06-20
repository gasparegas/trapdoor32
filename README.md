# TrapDoor32

```
  █▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀█
  █  T R A P D O O R █
  █  ░░░░░░░░░░░░░░░ █
  █     3  2   ░░░░  █
  █▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄█
```

## Overview

TrapDoor32 is an ESP32-based captive portal phish-testing framework designed for TTGO LoRa32 and similar boards. It provides a customizable login portal to capture credentials in a secure, educational environment.

- **Customizable UI**: Fully skinnable captive portal pages with support for static assets.
- **Credential Logging**: Captures usernames/emails and passwords, stores them in SPIFFS.
- **Admin Panel**: Access logged credentials via a protected admin interface.
- **Lightweight**: Minimal dependencies and efficient storage management.
- **Easy Setup**: One-file `main.cpp`, SPIFFS data folder, and PlatformIO configuration.

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/TrapDoor32.git
   cd TrapDoor32
   ```
2. Install PlatformIO dependencies:
   ```bash
   pio run
   ```
3. Upload to your TTGO LoRa32 (or compatible) board:
   ```bash
   pio run --target upload --environment ttgo-lora32-v1
   ```
4. Monitor serial output:
   ```bash
   pio device monitor -e ttgo-lora32-v1
   ```

## Usage

1. Power up the board; it will create an open WiFi SSID `TrapDoor32`.
2. Connect your device; the captive portal should automatically pop up.
3. Choose a social provider or email login, enter credentials.
4. Switch to the admin panel (at `/admin`) to review captured data.

## Project Structure

```
├── src/
│   └── main.cpp          # Core logic and portal handlers
├── data/
│   ├── captive.html      # Captive portal HTML
│   ├── style.css         # Shared styles
│   ├── script.js         # Client-side logic
│   └── icons/            # Social login icons
├── platformio.ini        # Build and environment settings
└── README.md             # Project documentation
```

## Contributing

Feel free to open issues, suggest features, or submit pull requests. All code should be properly documented in English.

## License

This project is released under the MIT License.

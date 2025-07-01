# TrapDoor32

```

█▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀█
█  T R A P D O O R █
█  ░░░░░░░░░░░░░░░ █
█     3  2   ░░░░  █
█▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄█

````

## Overview

TrapDoor32 is an ESP32-based captive-portal credential tester for TTGO LoRa32 (and similar) boards.  
It spins up an open WiFi AP, shows a customizable login UI to “capture” credentials, logs everything to SPIFFS, and lets you review/download data via a protected Admin Panel.

- **Customizable captive portal** with social-login style buttons  
- **Credential logging** (last 50 entries) in `/creds.json`  
- **Runtime SSID management** via Admin Panel (no code reflashing)  
- **Stats display** on the onboard TFT display  
- **Minimal dependencies** (TFT_eSPI, ESPAsyncWebServer, ArduinoJson, SPIFFS)

## Installation

1. **Clone & build**
   ```bash
   git clone https://github.com/yourusername/TrapDoor32.git
   cd TrapDoor32
   pio run
````

2. **Upload**

   ```bash
   pio run --target upload --environment ttgo-lora32-v1
   ```
3. **Monitor** (optional)

   ```bash
   pio device monitor --environment ttgo-lora32-v1
   ```

## Quick Start

1. **Power on** the board.
2. Connect to the open WiFi SSID (default: whatever you last saved, or `Free_WiFi` on first boot).
3. The captive portal will pop up automatically on most devices.
4. Choose a provider or email, enter any credentials—those get saved to SPIFFS.
5. View live stats on the TFT display (clients & creds).

## Admin Panel

All administrative actions live under the `/admin` path:

* **Download captured credentials** as JSON
* **Change the AP SSID** on-the-fly (takes effect immediately)

**Default admin credentials** (hard-coded in `main.cpp`):

```
User: admin
Pwd : 1234
```

### How it works

* **GET** `/admin`

  * HTTP Basic auth (`admin` / `1234`)
  * Serves `data/admin.html`, `data/admin.css`, `data/admin.js`
* **GET** `/admin/creds.json`

  * Authenticated download of captured `creds.json`
* **POST** `/admin/wifi`

  * Authenticated JSON body `{ "ssid": "NEW_NAME" }`
  * Saves the new SSID into `/config.json` and restarts the SoftAP

### Example Screenshots
# TrapDoor32

> Captive-portal phishing demonstrator for TTGO ESP32 with 1.14″ ST7789

…

## Admin Panel: Change SSID & Download Credentials

Navigate to [`http://192.168.4.1/admin`](http://192.168.4.1/admin) (default user `admin`, pwd `1234`) to:

1. **Download** your `creds.json` log  
2. **Change** the Wi-Fi SSID on-the-fly  
3. **Restart** the SoftAP under the new SSID automatically

---

## Screenshots

<table>
  <tr>
    <td align="center">
      **1. Connect to Free_WiFi**  
      <img src="docs/screenshots/1.png" width="200"/>
    </td>
    <td align="center">
      **2. Captive window opens**  
      <img src="docs/screenshots/2.png" width="200"/>
    </td>
    <td align="center">
      **3. Custom login page**  
      <img src="docs/screenshots/3.png" width="200"/>
    </td>
  </tr>
  <tr>
    <td align="center">
      **4. Select “Instagram”**  
      <img src="docs/screenshots/4.png" width="200"/>
    </td>
    <td align="center">
      **5. TrapDoor32 Admin**  
      <img src="docs/screenshots/5.png" width="200"/>
    </td>
    <td align="center">
      **6. Change SSID form**  
      <img src="docs/screenshots/6.png" width="200"/>
    </td>
  </tr>
</table>

---

## Project Layout

```
├── src/
│   └── main.cpp           # Core logic (captive portal + admin + TFT UI)
├── data/
│   ├── captive.html       # Captive portal page
│   ├── style.css
│   ├── script.js
│   ├── admin.html         # Admin Panel UI
│   ├── admin.css
│   ├── admin.js
│   ├── icons/             # Social login icons
│   └── screenshots/       # Example screenshots
├── platformio.ini         # Build settings
└── README.md              # Documentation
```

## TFT Display

* **Portrait** (vertical) mode
* **Header** with logo text and separator line
* Live refresh of:

  * **Users** = current connected stations
  * **Creds** = total captured entries

## Contributing

PRs and issues welcome! Please keep comments in English and only document non-trivial logic.

---

*Released under the MIT License.*

```
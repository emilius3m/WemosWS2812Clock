# Clock Project - Quick Guide for Beginners

This project is a **Wi-Fi LED ring clock** for ESP8266 (Wemos D1 mini) with:
- NTP time sync (internet time)
- Web interface for colors/brightness
- Saved settings (kept after reboot)

---

## 1) What you need

- 1x Wemos D1 mini (ESP8266)
- 1x WS2812 / NeoPixel ring (60 LEDs)
- USB cable
- PC with VS Code + PlatformIO extension
- Wi-Fi network with internet (for NTP)

---

## 2) Wiring

- LED ring **DIN** -> D1 on D1 mini
- LED ring **5V** -> 5V
- LED ring **GND** -> GND

> Important: if the ring is unstable, use an external 5V power supply and common GND.

---

## 3) First flash (upload firmware)

1. Open this project folder in VS Code.
2. Open PlatformIO sidebar.
3. Select environment `d1_mini` from [`platformio.ini`](platformio.ini).
4. Click **Build** then **Upload**.

If upload fails, check USB cable/driver and COM port.

---

## 4) First startup (Wi-Fi setup)

1. Power on the board.
2. If Wi-Fi is not configured, the device starts Access Point: **Clock-Setup**.
3. Connect with phone/PC to `Clock-Setup`.
4. Captive portal appears: choose your home Wi-Fi and save.
5. Device reconnects and prints its IP on serial (example `192.168.1.123`).

---

## 5) Open web control page

- From browser, open: `http://<device-ip>/`
- Example: `http://192.168.1.123/`

From this page you can:
- change colors (quadrants/hour/minute/second)
- set brightness
- show/hide quadrants
- set custom NTP server

Settings are saved in EEPROM and restored after reboot.

---

## 6) NTP and timezone

- Timezone is Europe/Rome (CET/CEST, automatic daylight saving).
- Default NTP server: `pool.ntp.org`
- You can change NTP server from web UI.

If sync fails:
- verify internet connection on your Wi-Fi
- try another NTP server (`time.google.com`, `time.cloudflare.com`)

---

## 7) Project folders

- [`src/main.cpp`](src/main.cpp): main firmware
- [`platformio.ini`](platformio.ini): board and dependencies
- [`lib/`](lib/): optional custom libraries
- [`include/`](include/): optional header files
- [`test/`](test/): optional tests

You can leave [`lib/`](lib/) empty if you don't use custom libraries.

# 🚀 ESP32 Hardware Showcase & 16-Band Spectrum Analyzer

An ultra-fast, modular, ESP-IDF v5.x C hardware showcase and 16-band audio spectrum analyzer for **ESP32-WROOM (4MB Flash)**.

Includes a **GitHub Pages Web Flasher** powered by **ESP Web Tools**, allowing instant single-click firmware flashing straight from WebSerial-compatible browsers (Google Chrome, Microsoft Edge, Brave, Opera) without installing local toolchains!

---

## ⚡ Live Web Serial Flasher (GitHub Pages)

You can flash the latest compiled firmware directly to your ESP32 board from your browser:

👉 **[Open Browser Web Flasher](https://user.github.io/esp32_hardware_showcase/)**

1. Connect your ESP32 board via USB to your PC.
2. Open the link above in Chrome or Edge.
3. Click **INSTALL** and select your ESP32 COM port.

---

## 🎛️ Hardware Wiring Diagram

### 1. ST7789 2.4" SPI Display (240x240 RGB565)
| Screen Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **MOSI** | GPIO 13 | SPI Data Output |
| **SCLK** | GPIO 14 | SPI Clock |
| **CS** | GPIO 15 | SPI Chip Select |
| **DC** | GPIO 2 | Data / Command Select |
| **PWR** | GPIO 21 | Power Control (Active LOW) |
| **BL** | GPIO 19 | Backlight Dimming (LEDC PWM) |

### 2. INMP441 I2S Digital MEMS Microphone
| INMP441 Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **BCLK** | GPIO 26 | Bit Clock |
| **WS (LRCLK)** | GPIO 25 | Word Select |
| **SD** | GPIO 22 | Data Input |
| **VDD / GND** | 3.3V / GND | Power |

### 3. T9 Capacitive Touch Sensor
| Touch Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **T9 Touch** | GPIO 32 | Capacitive Touch Pad (TOUCH_PAD_NUM9) |

---

## 🕹️ Interactive Touch Gestures (T9 / GPIO 32)

* **Single Tap:** Cycles UI Modes (`Mode 0` -> `Mode 1` -> `Mode 2` -> `Mode 3` -> `Mode 0`).
* **Double Tap:** Resets spectrum peak hold values in Mode 0 / resets 3D particle positions in Mode 1.
* **Long Press (>800ms):** Toggles backlight brightness between **100%** and **30%** via LEDC PWM.

---

## 🎨 4 Retro Nostalgic UI Modes

1. **Mode 0: 16-Band Vintage Hi-Fi Spectrum Analyzer**
   - 22.05kHz I2S DMA sampling with 512-point Cooley-Tukey FFT & Hann windowing.
   - Mint -> Gold -> Brick Red gradient bars, peak hold indicators, total RMS volume %, and transient Clap Detection.
2. **Mode 1: Synthwave 3D Wireframe Cube & Particles**
   - Hardware FPU floating-point matrix projections, bouncing neon particles, and 60 FPS counter.
3. **Mode 2: Retro CRT Terminal System Dashboard**
   - Core 0 & Core 1 CPU load %, Free SRAM / Minimum Heap, chip temperature, NVS status, and live Wi-Fi AP RSSI scanner.
4. **Mode 3: T9 Amber CRT Touch Oscilloscope**
   - Live filtered touch sensor signal wave renderer in Amber Phosphor style.

---

## 🛠️ Local Compilation & Flashing Instructions

```bash
# 1. Source ESP-IDF v5.x environment
. $HOME/esp/esp-idf/export.sh

# 2. Set chip target
idf.py set-target esp32

# 3. Build project
idf.py build

# 4. Flash and launch serial monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## 📦 Single Merged Ready-to-Flash Binary

For manual flashing via `esptool.py` at offset `0x0`:

```bash
esptool.py -p /dev/ttyUSB0 -b 921600 write_flash 0x0 release/esp32_hardware_showcase_merged_0x0.bin
```

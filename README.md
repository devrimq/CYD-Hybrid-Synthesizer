# 🎹 CYD Hybrid Synthesizer & Organon

[![Board](https://img.shields.io/badge/Board-ESP32--2432S035%20(CYD)-yellow.svg)](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20%2F%20PlatformIO-blue.svg)](https://platformio.org/)
[![Graphics](https://img.shields.io/badge/Library-LovyanGFX-green.svg)](https://github.com/lovyan03/LovyanGFX)
[![License](https://img.shields.io/badge/License-MIT-brightgreen.svg)](LICENSE)

<img width="720" height="695" alt="ORG ESP32" src="https://github.com/user-attachments/assets/7d90abd2-5029-437a-bd90-8d33b456701c" />


A custom touch-controlled hybrid synthesizer, electronic organ, and acoustic piano emulator built for the **Cheap Yellow Display (CYD)** ESP32 board (`ESP32-2432S035` / `ESP32-2432S028`).

This project features **hardware-level touch calibration**, dynamic octave shifting, custom sound envelope synthesis (decay/vibrato), and multiple instrument modes running on LovyanGFX.

---

## 🌟 Key Features

* **🎹 3 Instrument Modes:**
  * **ORGAN:** Continuous clean organ tones.
  * **SYNTH:** Dynamic vibrato/frequency modulation for rich electronic synthesizer sounds.
  * **PIANO:** Acoustic piano decay envelope modeling (simulates string damping to prevent coil overheating).
* **🎼 Octave Control (OCT- / OCT+):**
  * **Low (Pes):** Deep bass tones.
  * **Normal (Nor):** Standard pitch (C4-C5 range).
  * **High (Tiz):** Lead synth / soprano notes.
* **🎯 Hardware Touch Calibration:**
  * Custom 8-element voltage matrix loaded directly into hardware (`lcd.setTouchCalibrate(calData)`), offering pixel-perfect responsiveness on resistive displays.
* **🔊 Audio Protection:**
  * Integrated acoustic decay timeouts to avoid DC square wave heating on small 8 Ohm speakers.

---

## 📌 Hardware Pinout & Architecture

| Component | Pin / Bus | Details |
| :--- | :--- | :--- |
| **MCU** | ESP32-WROOM | Dual-Core @ 240MHz |
| **Display** | ST7796S (SPI) | $480 \times 320$ Resolution (Landscape Mode) |
| **SPI SCLK / MOSI / MISO** | GPIO 14 / GPIO 13 / GPIO 12 | HSPI Host |
| **Display CS / DC** | GPIO 15 / GPIO 2 | SPI CS & Data/Command |
| **Display Backlight (PWM)** | GPIO 27 | PWM Channel 7 |
| **Touch Controller** | XPT2046 (SPI) | Dedicated Touch SPI Bus |
| **Touch SCLK / MOSI / MISO**| GPIO 25 / GPIO 32 / GPIO 39 | Touch SPI Lines |
| **Touch CS** | GPIO 33 | Active Low |
| **Audio Output** | GPIO 26 | Built-in Amplifier / Speaker Output |

---

## 🎯 Verified Touch Calibration Matrix

To skip manual software inversion or axis re-mapping, the following matrix is directly embedded in code:

```cpp
// 8-element hardware calibration matrix for ST7796S + XPT2046 (Landscape)
uint16_t calData[8] = {3842, 3882, 3847, 174, 378, 3803, 374, 145};

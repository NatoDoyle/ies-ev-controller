# EV Controller Interface: ESP32 port

My MSP430 (Energia) firmware, `../firmware.cpp`, moved to the ESP32-WROVER-E DevKit
(DevKitC-VE) with as few changes as possible. The code is in `src/main.cpp`. Each change
for the ESP32 is marked `ESP32:` with the reason, and each pin `#define` notes the MSP430
pin it replaces.

## Open it in VS Code or Cursor

1. Install one PlatformIO extension:
   - **VS Code:** Extensions panel → search "PlatformIO IDE" (publisher PlatformIO).
     VS Code also suggests it when you open this folder.
   - **Cursor:** the official PlatformIO IDE isn't in Cursor's extension store (Open VSX),
     so install **pioarduino IDE** (publisher pioarduino). It's a fork of PlatformIO IDE
     with the same buttons, and it reads the same `platformio.ini`. Ignore Cursor's
     suggestion for `platformio.platformio-ide` and any third-party "PlatformIO IDE" copies.
   - Only ever have one of them installed: not "PlatformIO IDE" and "pioarduino IDE" together.
2. **File → Open Folder… → this `ev-esp32` folder** (the one with `platformio.ini` in it,
   not the whole `IES` folder).
3. Wait for PlatformIO to finish setting up. The first build downloads the ESP32 toolchain
   and framework (a few GB, once).
4. In the status bar at the bottom of the window: **✓ Build**, **→ Upload**,
   **🔌 Serial Monitor** (9600 baud).

Problems:
- **"Failed to connect to ESP32" or "Wrong boot mode detected":** hold **BOOT**, tap **EN**,
  release **BOOT**, then upload again.
- **No serial port at all:** BOOT/EN won't help. Try another USB cable (some are
  charge-only) or another USB port, and install the CP210x USB-to-UART driver if the
  computer doesn't list the port.
- **A burst of garbage characters after reset or upload is normal.** It's the ESP32's own
  boot messages, sent at 115200 baud. Your program's output at 9600 follows.
- Close the serial monitor before running anything else that uses the port.

## Pin map

| Function | MSP430 | ESP32 GPIO |
|---|---|---|
| Light sensor (LDR) | P1_0 / A0 | 36 (VP) |
| Joystick centre button | P1_3 / A3 | 33 |
| Joystick up/down (motor / brake) | P1_4 / A4 | 34 |
| Joystick left/right | P1_5 / A5 | 39 (VN) |
| Temperature (LM35) | P1_7 / A7 | 35 |
| S1 gear up | P2_2 | 25 |
| S3 gear down | P2_3 | 26 |
| S4 left indicator | P2_4 | 27 |
| S2 right indicator | P2_5 | 13 |
| Motor (1 kΩ → TIP120 base) | P1_6 | 23 |
| Brake light (7-seg d) | P2_6 | 18 |
| Headlight (7-seg a) | P2_7 | 22 |
| Right indicator (7-seg b+c) | P2_0 | 21 |
| Left indicator (7-seg e+f) | P2_1 | 19 |

Wiring notes:
- **Joystick and LDR divider on 3V3, not 5 V.** The ESP32's pins take 3.3 V at most.
- The LM35 is powered from 5V, but its output is only about 10 mV per °C, so it's safe on GPIO35.
- Switches go between the pin and GND (internal pull-up, pressed = LOW), as on the MSP430.
- The analog pins (GPIO33, 34, 35, 36, 39) are all on ADC1, which keeps working if WiFi is ever added.

## What changed from the MSP430 version, and why

| MSP430 | ESP32 | Why |
|---|---|---|
| `P2_0`, `P1_6`… | GPIO numbers (21, 23…) | The ESP32 names pins by number |
| `analogRead(A3)` etc. | `analogRead10(JS_CENTRE_PIN)` etc. | `A3`, `A4`… are different pins on the ESP32 |
| 10-bit ADC, 0–1023 | `analogRead10()` reads calibrated millivolts and scales them to 0–1023 | The ESP32 ADC is 12-bit and not a straight line. Scaling keeps 550, 500, 220 and 0.322 meaning the same as before |
| `for (pin = 10; pin <= 13; …)` | One `pinMode(…, INPUT_PULLUP)` per switch | 10–13 are Energia's numbers for P2_2–P2_5. On the ESP32 they'd be different pins, and GPIO10–11 are wired to the flash chip |
| `analogReference(DEFAULT)` | Commented out | Not available on the ESP32; it already reads 0–3.3 V |
| Energia adds `Arduino.h` itself | `#include <Arduino.h>` | PlatformIO needs it in a `.cpp` file |

## How the outputs compare with the MSP430

`analogWrite` keeps its default 0–255 range on the ESP32, as on the MSP430, and any value of
255 or more is fully on.

- **Brake light:** exactly as before. Fully on while the stick reads 255–499, dimming below
  255, and off at full-down (the stick reads 0).
- **Motor:** fully on whenever the stick reads above 550. On the MSP430, values above 255
  overflowed Energia's 16-bit PWM timer. The result was an accidental ramp from about 15 %
  at 551 to 100 % at 766, then fully on from 767. On the ESP32 that range is simply fully
  on. To make the motor speed follow the stick properly, see idea 1 in
  `SUGGESTED_IMPROVEMENTS.md`.
- **Analog readings:** the ESP32 ADC can't measure much below 0.1 V. `analogRead10()`
  returns 0 there, like the MSP430, but readings from about 0.1 to 0.15 V are less
  precise. The LM35 gives 0.15 V at 15 °C, so temperatures below about 15 °C read wrongly
  (room temperature is fine).

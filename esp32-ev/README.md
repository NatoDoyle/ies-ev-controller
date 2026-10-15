# EV Controller Interface: ESP32-WROVER-E port

This is the IES mini-project ("EV Controller Interface"), rebuilt on an ESP32-WROVER-E DevKit (DevKitC-VE). The original was written for the MSP430 in Energia (`../firmware.cpp`).

| Part | Where | Runs on |
|---|---|---|
| Firmware | `esp32-ev/src`, `esp32-ev/include` | ESP32 (PlatformIO, Arduino framework) |
| Car logic (unit-tested) | `esp32-ev/lib/ev_logic` | ESP32 **and** PC (`pio test -e native`) |
| Web dashboard | `esp32-ev/web/index.html` (compiled into the firmware) | Phone or laptop browser over the ESP32's WiFi |
| Processing dashboard | `../dashboard/EV_Dashboard/EV_Dashboard.pde` | PC, over USB serial |

## Learning outcomes and where they are met

| # | Outcome | Where |
|---|---|---|
| 1 | Read ALL switch states | `readInputs()` in `main.cpp`, debounced in `debouncer.cpp`. All 5 switches are in the transmit array. |
| 2 | analogRead speed, steering, light, temp | `readInputs()`. Temperature comes from an LM35 via `analogReadMilliVolts`. |
| 3 | Transmit array | `buildTxArray()` / `formatTxLine()` in `telemetry.cpp` |
| 4 | Serial MCU ⇄ PC | `$EV` lines go to the PC; the PC sends `?` and the ESP32 replies with the `#EV` header |
| 5 | Dashboard GUI | Processing sketch, plus the web dashboard |
| 6 | Motor speed and brake light intensity | `motorDuty()` / `brakeDuty()` in `drive.cpp`, using `analogWrite` |
| 7 | Automatic headlights (LDR) | `Headlight` in `lights.cpp`, with hysteresis and a 1 s hold |
| 8 | Indicators with S4 / S2 | `Indicators` in `lights.cpp`: press to toggle, blinks at ~1.5 Hz |
| 9 | Gears with S1 / S3 | `shiftGear()` in `drive.cpp`: N,1–4, clamped at both ends |
| 10 | Commented code / paper design | Every file has the housekeeping header and line comments. Flowcharts are below. |

## Pin map (MSP430 → ESP32)

| Function | MSP430 | ESP32 GPIO | DevKit label | Mode |
|---|---|---|---|---|
| LDR (auto lights) | P1_0 / A0 | 36 | VP | analog |
| Joystick VER (throttle / brake) | P1_4 | 34 | 34 | analog |
| Joystick HOR (steering) | P1_5 | 39 | VN | analog |
| Temperature (LM35) | P1_7 / A7 | 35 | 35 | analog (mV) |
| Joystick SEL (start / stop) | P1_3 | 33 | 33 | input, pull-up |
| S1 gear up | P2_2 | 25 | 25 | input, pull-up |
| S3 gear down | P2_3 | 26 | 26 | input, pull-up |
| S4 left indicator | P2_4 | 27 | 27 | input, pull-up |
| S2 right indicator | P2_5 | 13 | 13 | input, pull-up |
| Motor PWM (1k → TIP120 base) | P1_6 | 23 | 23 | PWM |
| Headlights (seg a) | P2_7 | 22 | 22 | output |
| Right indicator (seg b+c) | P2_0 | 21 | 21 | output |
| Left indicator (seg e+f) | P2_1 | 19 | 19 | output |
| Brake light (seg d) | P2_6 | 18 | 18 | PWM |

How the pins were chosen:
- **Analog pins use ADC1 only (GPIO32–39).** ADC2 can't be read while WiFi is on.
- **GPIO34–39 are input-only and have no pull-ups,** so they are only used for analog inputs.
- **Pins avoided:**
  - 0, 2, 5, 12 and 15: strapping pins that set the boot mode
  - 6–11: flash
  - 16 and 17: PSRAM on the WROVER
  - 1 and 3: USB serial
  - 14: pulses at boot

## Wiring changes from the MSP430 version

- **Joystick:** VCC to **3V3**, as before. SEL goes to GPIO33 and GND; no resistor is needed because the internal pull-up is used.
- **Switches:** each one goes between its GPIO and GND, as before.
- **LDR:** 10k from 3V3 to GPIO36, and the LDR from GPIO36 to GND. Dark gives a high reading.
- **7-segment display:**
  - Common cathode to GND.
  - Keep one resistor per segment (220–470 Ω).
  - b and c share GPIO21, and e and f share GPIO19. That is about 13 mA per pin, which is within limits.
- **Motor:**
  - GPIO23 → 1 kΩ → TIP120 base. The emitter goes to GND, the collector to the motor, and the flyback diode sits across the motor.
  - **Power the motor from its own 5 V supply and join its GND to the ESP32 GND.** If you must use the DevKit's 5V pin, put a 470 µF capacitor across the motor supply. Otherwise motor start-up plus WiFi can brown out and reset the ESP32.
- **Temperature (LM35):**
  - Hold the LM35 with the flat face towards you and the legs down. The pins are then **+Vs, Vout, GND** from left to right.
  - +Vs goes to the DevKit's **5V** pin, because the LM35 needs at least 4 V. Vout goes to **GPIO35** and GND to GND.
  - The output is 10 mV per °C, which is only 0.25 V at 25 °C, so it is safe for the ESP32.
  - It should read about room temperature (20–25 °C). If it reads about 70 °C, the sensor is really a TMP36. In that case, set `TEMP_IS_TMP36 true` in `include/config.h` and power it from 3V3.

## Behaviour

- **Start/stop:** press the joystick button to toggle the ignition. The ignition starts **off**.
  - While it is off, the motor is off and the gear is forced to N.
  - After switching on, the motor stays off until the throttle has been back at centre once, so the car can't lurch away.
- **Throttle and brake:**
  - Push the stick up for throttle and pull it down for brake.
  - The centre is measured at power-up, so **don't touch the stick while the board boots**.
  - Braking turns the motor off and lights the brake segment in proportion to the stick.
- **Gears:**
  - S1 shifts up and S3 shifts down, through N, 1, 2, 3, 4. There is no wrap-around.
  - Each gear caps the motor at 25 / 50 / 75 / 100 %. Full stick travel always uses the whole range up to the cap.
- **Indicators:**
  - S4 starts the left indicator and S2 the right; they blink.
  - Pressing the other side switches over, and pressing the same side again cancels.
- **Temperature:**
  - It is sent as tenths of a degree, so 234 means 23.4 °C.
  - At 40 °C and above (`OVERHEAT_DECI_C`), `hot` is set to 1 and both dashboards show a warning.
- **Headlights:**
  - They come on automatically when it is dark for 1 s and go off when it is light for 1 s.
  - While the headlights are on, the brake segment glows dimly as a tail light.

All thresholds are in `include/config.h`. Watch the `ldr` value on a dashboard while covering and uncovering the LDR, then set `LDR_ON` / `LDR_OFF` either side of the change.

## Transmit array

The transmit array is sent every 100 ms at **115200 baud** over USB, and the same line goes over WiFi:

```
$EV,seq,ign,gear,thr,brk,steer,ldr,head,tempdC,hot,indL,indR,lampL,lampR,motor,brake,s1,s2,s3,s4,js
```

| Index | Field | Range |
|---|---|---|
| 0 | seq | 0–9999 message counter |
| 1 | ign | ignition 0/1 |
| 2 | gear | 0 = N, 1–4 |
| 3 / 4 | thr / brk | throttle / brake % 0–100 |
| 5 | steer | −100 (left) … +100 (right) |
| 6 | ldr | raw 0–4095 (high = dark) |
| 7 | head | headlights 0/1 |
| 8 | tempdC | temperature × 10, −999 = no sensor |
| 9 | hot | overheat 0/1 |
| 10 / 11 | indL / indR | indicator switched on |
| 12 / 13 | lampL / lampR | indicator lamp lit right now (blinks) |
| 14 / 15 | motor / brake | PWM duty 0–255 |
| 16–20 | s1 s2 s3 s4 js | 1 = pressed |

- **Header line:** sent once at boot. Send `?` to get it again.
- **Other output:** lines starting with `#` are messages, not data.
- **Dashboards:** both dashboards ignore anything that doesn't start with `$EV`.

## Build, flash, run

You need PlatformIO (`brew install platformio`, or the VS Code extension).

```sh
cd esp32-ev
pio test -e native        # run the 30 unit tests on the PC (no board needed)
pio run                   # build the firmware
pio run -t upload         # flash the DevKit (hold BOOT if it won't connect)
pio device monitor        # see the "#EV" header then "$EV,..." lines (Ctrl+C to quit)
```

**Web dashboard:**
1. Join the WiFi network **EV-Dashboard** (password `evcontrol`, set in `config.h`).
2. Open **http://192.168.4.1**.
3. If your phone says "no internet", choose "stay connected".

To try the page with no board, open `web/index.html` directly in a browser, or add `?demo=1` to the URL.

**Processing dashboard:**
1. Install Processing 4 (`brew install --cask processing`).
2. Open `../dashboard/EV_Dashboard/EV_Dashboard.pde` and press Run.
3. **Close `pio device monitor` first,** because only one program can use the port.
4. Keys: `P` next port, `D` demo mode, `H` ask for the header.

## Flowcharts (for the logbook)

**Main loop:**
```
          ┌──────────────┐
          │    setup()   │  pins, PWM off, calibrate joystick, WiFi, send header
          └──────┬───────┘
                 ▼
   ┌──────► read inputs ────────── joystick, LDR, temp, 5 switches
   │             ▼
   │       vehicleUpdate ───────── debounce → ignition → joystick % → gears
   │             │                 → indicators → headlights → motor/brake duty
   │             ▼
   │       drive outputs ───────── analogWrite motor/brake, digitalWrite lamps
   │             ▼
   │      100 ms passed? ──no──┐
   │             │ yes         │
   │             ▼             │
   │     build transmit array  │
   │     Serial.print + WiFi   │
   │             ▼             │
   │      '?' received? ◄──────┘ ── yes → print header
   │             ▼
   └──────── web housekeeping
```

**Switch debounce (per switch, each loop):**
```
raw changed? ──yes──► restart settle timer
     ▼
raw ≠ accepted AND steady ≥ 30 ms? ──no──► no change
     │ yes
     ▼
accept new state ── is it a press? ──yes──► press edge (used once)
```

**Indicators:**
```
S4 edge: side==LEFT ? OFF : LEFT (restart blink)
S2 edge: side==RIGHT ? OFF : RIGHT (restart blink)
lamp = side≠OFF AND ((now − start) / 333) is even
```

**Headlights:**
```
lights OFF and ldr > LDR_ON  for 1 s → ON
lights ON  and ldr < LDR_OFF for 1 s → OFF
otherwise keep the current state
```

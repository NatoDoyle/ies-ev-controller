/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  ESP32-WROVER-E DevKit (DevKitC-VE).

  Description:
  Pin map for the EV Controller. Each line shows the MSP430 pin it replaces.
  Rules used when picking ESP32 pins:
   - Analog inputs on ADC1 (GPIO32-39) only - ADC2 stops working with WiFi on.
   - GPIO34-39 are input only with NO internal pull-ups -> analog inputs only.
   - Avoid strapping pins 0/2/5/12/15 (boot mode), 6-11 (flash),
     16/17 (PSRAM on the WROVER), 1/3 (USB serial) and 14 (JTAG, pulses at boot).

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  Switches wire between the pin and GND (internal pull-up, pressed = LOW).
*/
#ifndef EV_PINS_H                     // include guard
#define EV_PINS_H                     // mark as included

// ---- Analog inputs (ADC1, 12 bit 0..4095) --------------------------------
#define PIN_LDR        36             // P1_0 / A0 -> GPIO36 (VP, A0)  light sensor
#define PIN_JS_VER     34             // P1_4 / A4 -> GPIO34 (A6)      joystick up/down = throttle/brake
#define PIN_JS_HOR     39             // P1_5 / A5 -> GPIO39 (VN, A3)  joystick left/right = steering
#define PIN_TEMP       35             // P1_7 / A7 -> GPIO35 (A7)      temperature (LM35 output)

// ---- Digital inputs (INPUT_PULLUP, pressed = LOW) -------------------------
#define PIN_JS_SEL     33             // P1_3      -> GPIO33  joystick button = start/stop
#define PIN_S1         25             // P2_2      -> GPIO25  S1 gear up
#define PIN_S3         26             // P2_3      -> GPIO26  S3 gear down
#define PIN_S4         27             // P2_4      -> GPIO27  S4 left indicator
#define PIN_S2         13             // P2_5      -> GPIO13  S2 right indicator

// ---- Outputs --------------------------------------------------------------
#define PIN_MOTOR      23             // P1_6      -> GPIO23  PWM -> 1k -> TIP120 base
#define PIN_HEAD       22             // P2_7      -> GPIO22  7-seg a   = headlights
#define PIN_IND_R      21             // P2_0      -> GPIO21  7-seg b+c = right indicator
#define PIN_IND_L      19             // P2_1      -> GPIO19  7-seg e+f = left indicator
#define PIN_BRAKE      18             // P2_6      -> GPIO18  7-seg d   = brake light (PWM)

#endif                                // EV_PINS_H

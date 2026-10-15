/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.1

  Hardware:
  ESP32-WROVER-E DevKit (DevKitC-VE).

  Description:
  Every number you might want to tune, in one place. main.cpp copies these
  into the VehicleConfig used by lib/ev_logic.

  History:
  V1.0 :- 20261008
  First version.
  V1.1 :- 20261015
  LM35 temperature sensor fitted (TEMP_ENABLED true).

  Configuration:
  Tune LDR_ON / LDR_OFF using the "ldr" value shown on either dashboard.
*/
#ifndef EV_CONFIG_H                   // include guard
#define EV_CONFIG_H                   // mark as included

// ---- Serial / transmit array ----------------------------------------------
#define SERIAL_BAUD        115200     // must match the Processing sketch and platformio.ini
#define TX_PERIOD_MS       100        // send the transmit array every 100 ms (10 Hz)

// ---- Inputs ----------------------------------------------------------------
#define ADC_SAMPLES        8          // readings averaged per analog input
#define CAL_SAMPLES        32         // readings averaged for joystick centre at boot
#define DEBOUNCE_MS        30         // switch must be steady this long
#define JS_DEADZONE        150        // +/- ADC counts ignored around the centre
#define JS_CENTRE_DEFAULT  1900       // typical centre (3V3 joystick on the ESP32 ADC)
#define JS_CENTRE_MIN      1500       // lower limit for a believable centre (default - 400)
#define JS_CENTRE_MAX      2300       // upper limit for a believable centre (default + 400)
#define THROTTLE_INVERT    false      // set true if pushing UP brakes on your joystick
#define STEER_INVERT       false      // set true if steering is back to front

// ---- Motor and brake light (analogWrite 0..255) ------------------------------
#define GEAR_MAX_DUTY      {0, 64, 128, 191, 255}  // N, 1, 2, 3, 4 = 0/25/50/75/100 %
#define MOTOR_MIN_DUTY     40         // lowest duty that still turns the motor
#define TAIL_DUTY          25         // dim tail light (~10 %) while headlights are on

// ---- Lights ----------------------------------------------------------------
#define LDR_ON             2600       // reading above this = dark -> headlights on
#define LDR_OFF            2200       // reading below this = light -> headlights off
#define LDR_HOLD_MS        1000       // dark/light must last 1 s before switching
#define BLINK_HALF_MS      333        // indicator on/off time (~1.5 Hz)

// ---- Temperature ---------------------------------------------------------------
#define TEMP_ENABLED       true       // LM35 wired to PIN_TEMP (false = no sensor, sends -999)
#define TEMP_IS_TMP36      false      // false = LM35 (power from 5V), true = TMP36 (reads ~50 C too hot as LM35)
#define OVERHEAT_DECI_C    400        // 40.0 C overheat warning

// ---- WiFi web dashboard -----------------------------------------------------
#define WEB_ENABLED        true       // false = serial dashboard only
#define AP_SSID            "EV-Dashboard"  // network name the ESP32 creates
#define AP_PASS            "evcontrol"     // 8+ characters (change it!)

#endif                                // EV_CONFIG_H

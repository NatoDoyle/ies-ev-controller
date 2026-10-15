/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  The whole "car" in one place. vehicleUpdate() takes the raw inputs
  (joystick, LDR, switches) and the time, and works out every output
  (motor duty, brake light, headlight, indicators, gear, ignition).
  Because it does not touch any pins it can be unit tested on the PC.

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  main.cpp fills VehicleConfig from the #defines in include/config.h.
*/
#ifndef EV_VEHICLE_H                  // include guard
#define EV_VEHICLE_H                  // mark as included

#include <stdint.h>                   // fixed width integer types
#include "debouncer.h"                // switch debouncing
#include "drive.h"                    // joystick, gears, motor, brake maths
#include "lights.h"                   // headlight and indicators
#include "sensors.h"                  // temperature conversion

struct VehicleConfig {                // every tunable number (defaults shown)
  uint16_t debounceMs      = 30;      // switch settle time
  int      jsDead          = 150;     // joystick deadzone (ADC counts)
  int      jsCentreDefault = 1900;    // typical centre if calibration fails
  int      jsCentreMin     = 1500;    // lowest believable centre
  int      jsCentreMax     = 2300;    // highest believable centre
  bool     throttleInvert  = false;   // true if "up" reads low on your joystick
  bool     steerInvert     = false;   // true if "left" reads high on your joystick
  uint8_t  gearMaxDuty[GEAR_TOP + 1] = {0, 64, 128, 191, 255};  // N,1,2,3,4 = 0/25/50/75/100 %
  uint8_t  motorMinDuty    = 40;      // lowest duty that still turns the motor
  uint8_t  tailDuty        = 25;      // dim tail light (~10 %) with headlights on
  int      ldrOn           = 2600;    // LDR reading above this = dark
  int      ldrOff          = 2200;    // LDR reading below this = light
  uint32_t ldrHoldMs       = 1000;    // dark/light must last this long
  uint16_t blinkHalfMs     = 333;     // indicator half period (~1.5 Hz)
  bool     tempEnabled     = false;   // temperature sensor fitted?
  bool     tempIsTmp36     = false;   // false = LM35, true = TMP36
  int16_t  overheatDeciC   = 400;     // 40.0 C overheat warning
};

struct Inputs {                       // one snapshot of all the raw inputs
  int      ver = 0;                   // joystick up/down ADC (throttle/brake)
  int      hor = 0;                   // joystick left/right ADC (steering)
  int      ldr = 0;                   // light sensor ADC
  uint32_t tempMv = 0;                // temperature sensor millivolts
  bool     s1 = false;                // S1 gear up      (true = pressed)
  bool     s2 = false;                // S2 right indicator
  bool     s3 = false;                // S3 gear down
  bool     s4 = false;                // S4 left indicator
  bool     js = false;                // joystick button (start/stop)
};

struct VehicleState {                 // everything the car knows
  VehicleConfig cfg;                  // settings in use
  AxisCal   verCal;                   // throttle axis calibration
  AxisCal   horCal;                   // steering axis calibration
  Debouncer s1, s2, s3, s4, js;       // one debouncer per switch
  Headlight head;                     // automatic headlights
  Indicators ind;                     // indicators

  bool    ignition  = false;          // car started (JS button toggles)
  bool    armed     = false;          // throttle seen at zero since ignition on
  int     gear      = GEAR_N;         // 0 = N, 1..4
  int     thrPct    = 0;              // throttle 0..100 %
  int     brkPct    = 0;              // brake 0..100 %
  int     steerPct  = 0;              // steering -100 (left) .. +100 (right)
  int     ldrRaw    = 0;              // last LDR reading
  bool    headOn    = false;          // headlights on
  int16_t tempDeciC = TEMP_NA;        // temperature in tenths of C
  bool    hot       = false;          // overheat warning
  uint8_t motor     = 0;              // motor PWM duty 0..255
  uint8_t brake     = 0;              // brake light PWM duty 0..255
};

// Set up the state. verRaw/horRaw are the averaged joystick readings at boot.
void vehicleInit(VehicleState& st, const VehicleConfig& cfg, int verRaw, int horRaw);

// Run one step of the car logic using the latest inputs and time.
void vehicleUpdate(VehicleState& st, const Inputs& in, uint32_t nowMs);

#endif                                // EV_VEHICLE_H

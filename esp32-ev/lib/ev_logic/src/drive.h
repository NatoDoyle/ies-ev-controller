/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Drive train maths: joystick calibration and scaling, gearbox shifting,
  motor PWM duty and brake light PWM duty.
  The ESP32 ADC is 12 bit (0..4095) and analogWrite() takes 0..255.

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  Numbers come from include/config.h via VehicleConfig (vehicle.h).
*/
#ifndef EV_DRIVE_H                    // include guard
#define EV_DRIVE_H                    // mark as included

#include <stdint.h>                   // fixed width integer types

const int ADC_MAX  = 4095;            // highest 12 bit ADC reading
const int GEAR_N   = 0;               // gear 0 is Neutral
const int GEAR_TOP = 4;               // highest gear

struct AxisCal {                      // calibration for one joystick axis
  int  centre;                        // ADC reading with the stick at rest
  int  dead;                          // +/- deadzone around the centre
  bool invert;                        // true = swap the direction
  bool calFailed;                     // true = boot calibration rejected, default used
};

// Build an AxisCal from the averaged boot reading. If the reading is outside
// lo..hi (someone was holding the stick at power-up) the default centre is used.
AxisCal calibrateAxis(int avgRaw, int defaultCentre, int lo, int hi, int dead, bool invert);

// Convert a raw ADC reading to -100..+100 %. 0 inside the deadzone. The two
// halves are scaled separately so both ends reach 100 % even off-centre.
int axisPercent(int raw, const AxisCal& cal);

// Move the gear one step on a press edge. Clamped to N..4 (no wrap round).
int shiftGear(int gear, bool upEdge, bool downEdge);

// Motor PWM duty (0..255). Neutral or no throttle = 0. Otherwise the whole
// throttle range is scaled from minDuty up to the cap for the current gear.
uint8_t motorDuty(int gear, int throttlePct, const uint8_t gearMaxDuty[GEAR_TOP + 1], uint8_t minDuty);

// Brake light PWM duty (0..255). Proportional to brake %, and never below the
// dim tail light level while the headlights are on.
uint8_t brakeDuty(int brakePct, bool headOn, uint8_t tailDuty);

#endif                                // EV_DRIVE_H

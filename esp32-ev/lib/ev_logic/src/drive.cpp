/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Implementation of the drive train functions (see drive.h).

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  None.
*/
#include "drive.h"                    // function declarations

static int clampInt(int v, int lo, int hi) {  // keep v inside lo..hi
  if (v < lo) return lo;              // too small -> lower limit
  if (v > hi) return hi;              // too big -> upper limit
  return v;                           // already in range
}

AxisCal calibrateAxis(int avgRaw, int defaultCentre, int lo, int hi, int dead, bool invert) {
  AxisCal cal;                        // result
  cal.dead = dead;                    // copy the deadzone
  cal.invert = invert;                // copy the direction setting
  if (avgRaw < lo || avgRaw > hi) {   // reading is not plausible for a centred stick
    cal.centre = defaultCentre;       // fall back to the typical centre value
    cal.calFailed = true;             // flag it so it can be reported
  } else {                            // reading looks like a centred stick
    cal.centre = avgRaw;              // use the measured centre
    cal.calFailed = false;            // calibration OK
  }
  return cal;                         // give the calibration back
}

int axisPercent(int raw, const AxisCal& cal) {
  int d = raw - cal.centre;           // distance from the centre (signed)
  int pct = 0;                        // result, 0 = centred
  if (d > cal.dead) {                 // pushed past the deadzone on the + side
    int span = ADC_MAX - cal.centre - cal.dead;          // travel available on + side
    pct = (span > 0) ? (d - cal.dead) * 100 / span : 100; // scale to 0..100
  } else if (d < -cal.dead) {         // pushed past the deadzone on the - side
    int span = cal.centre - cal.dead;                     // travel available on - side
    pct = (span > 0) ? (d + cal.dead) * 100 / span : -100; // scale to -100..0
  }
  pct = clampInt(pct, -100, 100);     // never outside -100..100
  return cal.invert ? -pct : pct;     // swap direction if asked
}

int shiftGear(int gear, bool upEdge, bool downEdge) {
  if (upEdge) gear++;                 // S1 pressed -> up one gear
  if (downEdge) gear--;               // S3 pressed -> down one gear
  return clampInt(gear, GEAR_N, GEAR_TOP);  // stay in N..4, no wrap round
}

uint8_t motorDuty(int gear, int throttlePct, const uint8_t gearMaxDuty[GEAR_TOP + 1], uint8_t minDuty) {
  if (gear <= GEAR_N || gear > GEAR_TOP) return 0;   // Neutral (or bad gear) -> motor off
  if (throttlePct <= 0) return 0;     // no throttle -> motor off
  throttlePct = clampInt(throttlePct, 0, 100);       // limit to 0..100 %
  int cap = gearMaxDuty[gear];        // highest duty allowed in this gear
  int lo = (minDuty < cap) ? minDuty : cap;          // start point, never above the cap
  return (uint8_t)(lo + (cap - lo) * throttlePct / 100);  // scale throttle into lo..cap
}

uint8_t brakeDuty(int brakePct, bool headOn, uint8_t tailDuty) {
  brakePct = clampInt(brakePct, 0, 100);             // limit to 0..100 %
  int duty = brakePct * 255 / 100;    // proportional brake light brightness
  int floor = headOn ? tailDuty : 0;  // dim tail light when headlights are on
  return (uint8_t)(duty > floor ? duty : floor);     // brighter of the two
}

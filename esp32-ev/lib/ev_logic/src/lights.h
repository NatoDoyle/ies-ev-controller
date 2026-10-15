/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Headlight: automatic on/off from the LDR reading. The LDR sits in a
  divider (10k to 3V3, LDR to GND) so DARK = HIGH reading. Two thresholds
  (hysteresis) plus a hold time stop the lights flickering at dusk.

  Indicators: S4 / S2 toggle the left / right indicator. Pressing the other
  side switches over, pressing the same side again cancels. The lamp blinks
  using millis() timing (no delay()).

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  Thresholds and timings come from include/config.h via VehicleConfig.
*/
#ifndef EV_LIGHTS_H                   // include guard
#define EV_LIGHTS_H                   // mark as included

#include <stdint.h>                   // fixed width integer types

class Headlight {                     // automatic headlight controller
public:
  Headlight(int onLevel = 2600, int offLevel = 2200, uint32_t holdMs = 1000);  // thresholds + hold time
  bool update(int ldrRaw, uint32_t nowMs);  // feed the LDR reading, returns lights on/off
  bool on() const;                    // current headlight state

private:
  int      onLevel_;                  // reading above this (darker) -> want lights ON
  int      offLevel_;                 // reading below this (brighter) -> want lights OFF
  uint32_t holdMs_;                   // the wanted state must last this long
  bool     on_;                       // current output
  bool     pending_;                  // a change is waiting for the hold time
  uint32_t pendingSince_;             // when the pending change started
};

enum IndSide {                        // which indicator is running
  IND_OFF = 0,                        // none
  IND_LEFT = 1,                       // left (S4)
  IND_RIGHT = 2                       // right (S2)
};

class Indicators {                    // left/right indicator controller
public:
  explicit Indicators(uint16_t halfPeriodMs = 333);  // lamp on time = off time
  void pressLeft(uint32_t nowMs);     // S4 press edge
  void pressRight(uint32_t nowMs);    // S2 press edge
  void update(uint32_t nowMs);        // work out the blink state
  IndSide side() const;               // which side is active
  bool leftActive() const;            // left indicator switched on
  bool rightActive() const;           // right indicator switched on
  bool leftLamp() const;              // left lamp lit right now
  bool rightLamp() const;             // right lamp lit right now

private:
  void press(IndSide s, uint32_t nowMs);  // shared toggle logic
  uint16_t halfMs_;                   // half of one blink period
  IndSide  side_;                     // active side
  uint32_t startMs_;                  // when blinking started (lamp on first)
  bool     lamp_;                     // lamp lit this moment
};

#endif                                // EV_LIGHTS_H

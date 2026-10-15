/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic, no Arduino calls. Used by the ESP32 firmware and by the
  unit tests that run on the PC.

  Description:
  Software debouncer for one push switch. A mechanical switch "bounces" for a
  few milliseconds when pressed. This class only accepts a new state once the
  raw input has been steady for settleMs, and reports the press EDGE once, so
  holding a switch down does not repeat (the MSP430 version used delay(250)
  on the level, which kept changing gear while S1 was held).

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  Call update() every loop with the raw "is pressed" level and millis().
*/
#ifndef EV_DEBOUNCER_H                // include guard - stop double inclusion
#define EV_DEBOUNCER_H                // mark this header as included

#include <stdint.h>                   // fixed width integer types (uint32_t ...)

class Debouncer {                     // one debouncer per switch
public:
  explicit Debouncer(uint16_t settleMs = 30);    // settle time in milliseconds
  void update(bool rawPressed, uint32_t nowMs);  // feed raw level + current time
  bool pressed() const;                          // debounced level, true = held down
  bool pressEdge() const;                        // true for ONE update when a press is accepted

private:
  uint16_t settleMs_;                 // how long the input must be steady
  bool     stable_;                   // the accepted (debounced) state
  bool     lastRaw_;                  // raw state seen on the previous update
  uint32_t changedAt_;                // time the raw state last changed
  bool     edge_;                     // press edge flag for this update
};

#endif                                // EV_DEBOUNCER_H

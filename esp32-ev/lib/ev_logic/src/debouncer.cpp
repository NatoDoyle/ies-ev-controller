/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Implementation of the Debouncer class (see debouncer.h).

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  None.
*/
#include "debouncer.h"                // class declaration

Debouncer::Debouncer(uint16_t settleMs)  // constructor
    : settleMs_(settleMs),            // store the settle time
      stable_(false),                 // start "not pressed"
      lastRaw_(false),                // assume raw input not pressed
      changedAt_(0),                  // no change seen yet
      edge_(false) {}                 // no edge yet

void Debouncer::update(bool rawPressed, uint32_t nowMs) {
  edge_ = false;                      // edge only lasts for one update
  if (rawPressed != lastRaw_) {       // raw input changed (could be bounce)
    lastRaw_ = rawPressed;            // remember the new raw level
    changedAt_ = nowMs;               // restart the settle timer
  }
  if (rawPressed != stable_ &&                       // raw differs from accepted state
      (uint32_t)(nowMs - changedAt_) >= settleMs_) { // and has been steady long enough
    stable_ = rawPressed;             // accept the new state
    if (stable_) {                    // it is a press (not a release)
      edge_ = true;                   // report the press edge once
    }
  }
}

bool Debouncer::pressed() const { return stable_; }   // debounced level
bool Debouncer::pressEdge() const { return edge_; }   // one-shot press edge

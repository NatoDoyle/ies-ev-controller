/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Implementation of the Headlight and Indicators classes (see lights.h).

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  None.
*/
#include "lights.h"                   // class declarations

// ------------------------------------------------------------- Headlight ---

Headlight::Headlight(int onLevel, int offLevel, uint32_t holdMs)
    : onLevel_(onLevel),              // dark threshold
      offLevel_(offLevel),            // bright threshold
      holdMs_(holdMs),                // hold time
      on_(false),                     // lights start off
      pending_(false),                // no change waiting
      pendingSince_(0) {}             // no timer running

bool Headlight::update(int ldrRaw, uint32_t nowMs) {
  bool want = on_;                    // by default keep the current state
  if (!on_ && ldrRaw > onLevel_) want = true;    // off and now dark -> want on
  if (on_ && ldrRaw < offLevel_) want = false;   // on and now bright -> want off
  if (want == on_) {                  // nothing to change
    pending_ = false;                 // cancel any waiting change
  } else if (!pending_) {             // a change has just been requested
    pending_ = true;                  // start waiting
    pendingSince_ = nowMs;            // remember when
  } else if ((uint32_t)(nowMs - pendingSince_) >= holdMs_) {  // waited long enough
    on_ = want;                       // apply the change
    pending_ = false;                 // done waiting
  }
  return on_;                         // report the lights state
}

bool Headlight::on() const { return on_; }       // current state

// ------------------------------------------------------------ Indicators ---

Indicators::Indicators(uint16_t halfPeriodMs)
    : halfMs_(halfPeriodMs ? halfPeriodMs : 1),  // never 0 (would divide by 0)
      side_(IND_OFF),                 // indicators start off
      startMs_(0),                    // no blink started
      lamp_(false) {}                 // lamps off

void Indicators::press(IndSide s, uint32_t nowMs) {
  if (side_ == s) {                   // same side pressed again
    side_ = IND_OFF;                  // cancel the indicator
  } else {                            // off, or the other side was running
    side_ = s;                        // switch to the pressed side
    startMs_ = nowMs;                 // restart the blink so the lamp lights at once
  }
  update(nowMs);                      // refresh the lamp straight away
}

void Indicators::pressLeft(uint32_t nowMs) { press(IND_LEFT, nowMs); }    // S4
void Indicators::pressRight(uint32_t nowMs) { press(IND_RIGHT, nowMs); }  // S2

void Indicators::update(uint32_t nowMs) {
  if (side_ == IND_OFF) {             // nothing running
    lamp_ = false;                    // lamps off
    return;                           // done
  }
  uint32_t elapsed = nowMs - startMs_;            // unsigned maths survives millis() wrap
  lamp_ = ((elapsed / halfMs_) % 2) == 0;         // even half periods = lamp on
}

IndSide Indicators::side() const { return side_; }                         // active side
bool Indicators::leftActive() const { return side_ == IND_LEFT; }          // left running
bool Indicators::rightActive() const { return side_ == IND_RIGHT; }        // right running
bool Indicators::leftLamp() const { return side_ == IND_LEFT && lamp_; }   // left lamp lit
bool Indicators::rightLamp() const { return side_ == IND_RIGHT && lamp_; } // right lamp lit

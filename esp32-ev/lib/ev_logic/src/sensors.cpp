/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Implementation of the temperature functions (see sensors.h).

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  None.
*/
#include "sensors.h"                  // declarations

int16_t tempDeciC(uint32_t millivolts, bool isTmp36) {
  int32_t dC = (int32_t)millivolts;   // LM35: 1 mV = 0.1 C, so mV is already deci-C
  if (isTmp36) dC -= 500;             // TMP36 has a 500 mV offset at 0 C
  if (dC > 32767) dC = 32767;         // keep inside int16_t
  return (int16_t)dC;                 // tenths of a degree C
}

bool overheat(int16_t deciC, int16_t limitDeciC) {
  if (deciC == TEMP_NA) return false; // no sensor -> never overheated
  return deciC >= limitDeciC;         // at or above the limit
}

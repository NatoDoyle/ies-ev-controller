/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Temperature conversion. Not fitted yet (TEMP_ENABLED 0 in config.h) but
  ready for when it is. Temperatures are kept in tenths of a degree (deci-C)
  so they fit in the integer transmit array: 23.4 C -> 234.
    LM35  : 10 mV per C, 0 mV at 0 C       -> deciC = mV
    TMP36 : 10 mV per C, 500 mV at 0 C     -> deciC = mV - 500

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  None.
*/
#ifndef EV_SENSORS_H                  // include guard
#define EV_SENSORS_H                  // mark as included

#include <stdint.h>                   // fixed width integer types

const int16_t TEMP_NA = -999;         // sent when no temperature sensor is fitted

int16_t tempDeciC(uint32_t millivolts, bool isTmp36);  // mV -> tenths of a degree C
bool overheat(int16_t deciC, int16_t limitDeciC);      // true when at/over the limit

#endif                                // EV_SENSORS_H

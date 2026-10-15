/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  The TRANSMIT ARRAY (learning outcome 3). Every value the dashboards need is
  copied into one int16_t array, then sent as one text line:
      $EV,seq,ign,gear,thr,brk,steer,ldr,head,tempdC,hot,indL,indR,lampL,lampR,motor,brake,s1,s2,s3,s4,js\n
  The same line goes to the PC over USB serial (Processing dashboard) and to
  phones over WiFi (web dashboard). A header line starting "#EV" names the
  fields. Lines that don't start "$EV" are ignored by both dashboards.

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  If you add a field, update TX_COUNT, TxField, the header, and BOTH
  dashboards (web/index.html and EV_Dashboard.pde).
*/
#ifndef EV_TELEMETRY_H                // include guard
#define EV_TELEMETRY_H                // mark as included

#include <stddef.h>                   // size_t
#include <stdint.h>                   // fixed width integer types
#include "vehicle.h"                  // VehicleState

enum TxField {                        // position of each value in the array
  TX_SEQ = 0,                         // message counter 0..9999
  TX_IGN,                             // ignition 0/1
  TX_GEAR,                            // 0 = N, 1..4
  TX_THR,                             // throttle %
  TX_BRK,                             // brake %
  TX_STEER,                           // steering -100..100
  TX_LDR,                             // LDR raw 0..4095
  TX_HEAD,                            // headlights 0/1
  TX_TEMP,                            // temperature deci-C (-999 = none)
  TX_HOT,                             // overheat 0/1
  TX_IND_L,                           // left indicator active 0/1
  TX_IND_R,                           // right indicator active 0/1
  TX_LAMP_L,                          // left lamp lit now 0/1
  TX_LAMP_R,                          // right lamp lit now 0/1
  TX_MOTOR,                           // motor PWM duty 0..255
  TX_BRAKE,                           // brake light PWM duty 0..255
  TX_S1,                              // S1 pressed 0/1
  TX_S2,                              // S2 pressed 0/1
  TX_S3,                              // S3 pressed 0/1
  TX_S4,                              // S4 pressed 0/1
  TX_JS,                              // joystick button pressed 0/1
  TX_COUNT                            // number of fields (21)
};

const size_t TX_LINE_MAX = 160;       // buffer big enough for the longest line

// Fill the transmit array from the vehicle state.
void buildTxArray(const VehicleState& st, uint16_t seq, int16_t tx[TX_COUNT]);

// Turn the array into "$EV,...\n". Returns the length, or -1 if buf is too small.
int formatTxLine(const int16_t tx[TX_COUNT], char* buf, size_t len);

// The "#EV,..." header line naming every field (ends in \n).
const char* txHeader();

#endif                                // EV_TELEMETRY_H

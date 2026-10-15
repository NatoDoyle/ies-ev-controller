/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Implementation of the transmit array functions (see telemetry.h).

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  None.
*/
#include "telemetry.h"                // declarations
#include <stdio.h>                    // snprintf

void buildTxArray(const VehicleState& st, uint16_t seq, int16_t tx[TX_COUNT]) {
  tx[TX_SEQ]    = (int16_t)(seq % 10000);          // counter, wraps at 9999
  tx[TX_IGN]    = st.ignition;                     // ignition
  tx[TX_GEAR]   = (int16_t)st.gear;                // gear
  tx[TX_THR]    = (int16_t)st.thrPct;              // throttle %
  tx[TX_BRK]    = (int16_t)st.brkPct;              // brake %
  tx[TX_STEER]  = (int16_t)st.steerPct;            // steering %
  tx[TX_LDR]    = (int16_t)st.ldrRaw;              // LDR raw
  tx[TX_HEAD]   = st.headOn;                       // headlights
  tx[TX_TEMP]   = st.tempDeciC;                    // temperature
  tx[TX_HOT]    = st.hot;                          // overheat
  tx[TX_IND_L]  = st.ind.leftActive();             // left indicator on
  tx[TX_IND_R]  = st.ind.rightActive();            // right indicator on
  tx[TX_LAMP_L] = st.ind.leftLamp();               // left lamp lit
  tx[TX_LAMP_R] = st.ind.rightLamp();              // right lamp lit
  tx[TX_MOTOR]  = st.motor;                        // motor duty
  tx[TX_BRAKE]  = st.brake;                        // brake light duty
  tx[TX_S1]     = st.s1.pressed();                 // switch states (debounced)
  tx[TX_S2]     = st.s2.pressed();                 //  "
  tx[TX_S3]     = st.s3.pressed();                 //  "
  tx[TX_S4]     = st.s4.pressed();                 //  "
  tx[TX_JS]     = st.js.pressed();                 //  "
}

int formatTxLine(const int16_t tx[TX_COUNT], char* buf, size_t len) {
  int n = snprintf(buf, len, "$EV");  // start marker
  for (int i = 0; i < TX_COUNT; i++) {             // every field in order
    if (n < 0 || (size_t)n >= len) return -1;      // ran out of room
    n += snprintf(buf + n, len - n, ",%d", tx[i]); // comma then the value
  }
  if (n < 0 || (size_t)n + 1 >= len) return -1;    // no room for the newline
  buf[n++] = '\n';                    // end of line
  buf[n] = '\0';                      // end of string
  return n;                           // characters written (without the \0)
}

const char* txHeader() {
  return "#EV,seq,ign,gear,thr,brk,steer,ldr,head,tempdC,hot,"
         "indL,indR,lampL,lampR,motor,brake,s1,s2,s3,s4,js\n";  // field names
}

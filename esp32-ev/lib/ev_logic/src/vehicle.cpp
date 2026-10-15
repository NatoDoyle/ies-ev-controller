/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Implementation of vehicleInit() and vehicleUpdate() (see vehicle.h).
  Order of work each update:
    1 debounce switches      2 ignition (JS)       3 joystick -> %
    4 gears (S1/S3)          5 indicators (S4/S2)  6 headlights (LDR)
    7 temperature            8 motor + brake light duty

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  None.
*/
#include "vehicle.h"                  // declarations

void vehicleInit(VehicleState& st, const VehicleConfig& cfg, int verRaw, int horRaw) {
  st = VehicleState();                // clear everything back to defaults
  st.cfg = cfg;                       // keep a copy of the settings
  st.verCal = calibrateAxis(verRaw, cfg.jsCentreDefault, cfg.jsCentreMin,
                            cfg.jsCentreMax, cfg.jsDead, cfg.throttleInvert);  // throttle centre
  st.horCal = calibrateAxis(horRaw, cfg.jsCentreDefault, cfg.jsCentreMin,
                            cfg.jsCentreMax, cfg.jsDead, cfg.steerInvert);     // steering centre
  st.s1 = Debouncer(cfg.debounceMs);  // gear up switch
  st.s2 = Debouncer(cfg.debounceMs);  // right indicator switch
  st.s3 = Debouncer(cfg.debounceMs);  // gear down switch
  st.s4 = Debouncer(cfg.debounceMs);  // left indicator switch
  st.js = Debouncer(cfg.debounceMs);  // joystick button
  st.head = Headlight(cfg.ldrOn, cfg.ldrOff, cfg.ldrHoldMs);  // headlight thresholds
  st.ind = Indicators(cfg.blinkHalfMs);                       // blink speed
}

void vehicleUpdate(VehicleState& st, const Inputs& in, uint32_t nowMs) {
  // 1 - debounce every switch
  st.s1.update(in.s1, nowMs);         // S1 gear up
  st.s2.update(in.s2, nowMs);         // S2 right indicator
  st.s3.update(in.s3, nowMs);         // S3 gear down
  st.s4.update(in.s4, nowMs);         // S4 left indicator
  st.js.update(in.js, nowMs);         // joystick button

  // 2 - ignition: each joystick press toggles start/stop
  if (st.js.pressEdge()) {            // button just pressed
    st.ignition = !st.ignition;       // toggle the ignition
    st.armed = false;                 // must return throttle to zero before driving
  }

  // 3 - joystick: up = throttle, down = brake, left/right = steering
  int ver = axisPercent(in.ver, st.verCal);   // -100..100
  st.thrPct = ver > 0 ? ver : 0;      // positive half = throttle
  st.brkPct = ver < 0 ? -ver : 0;     // negative half = brake
  st.steerPct = axisPercent(in.hor, st.horCal);  // steering

  // 4 - gearbox: only works with the ignition on, otherwise Neutral
  if (st.ignition) {                  // engine running
    st.gear = shiftGear(st.gear, st.s1.pressEdge(), st.s3.pressEdge());  // S1 up, S3 down
    if (!st.armed && st.thrPct == 0) st.armed = true;  // throttle back at zero -> safe to drive
  } else {                            // ignition off
    st.gear = GEAR_N;                 // force Neutral
  }

  // 5 - indicators: S4 left, S2 right
  if (st.s4.pressEdge()) st.ind.pressLeft(nowMs);   // toggle left
  if (st.s2.pressEdge()) st.ind.pressRight(nowMs);  // toggle right
  st.ind.update(nowMs);               // work out the blink

  // 6 - automatic headlights from the LDR
  st.ldrRaw = in.ldr;                 // keep the reading for the dashboard
  st.headOn = st.head.update(in.ldr, nowMs);  // hysteresis + hold time

  // 7 - temperature (only if a sensor is fitted)
  if (st.cfg.tempEnabled) {           // sensor fitted
    st.tempDeciC = tempDeciC(in.tempMv, st.cfg.tempIsTmp36);  // mV -> deci-C
  } else {                            // no sensor
    st.tempDeciC = TEMP_NA;           // "not available"
  }
  st.hot = overheat(st.tempDeciC, st.cfg.overheatDeciC);  // overheat warning

  // 8 - outputs: motor only when running, armed and not braking
  bool canDrive = st.ignition && st.armed && st.brkPct == 0;  // all conditions met
  st.motor = canDrive ? motorDuty(st.gear, st.thrPct, st.cfg.gearMaxDuty, st.cfg.motorMinDuty) : 0;
  st.brake = brakeDuty(st.brkPct, st.headOn, st.cfg.tailDuty);  // brake light brightness
}

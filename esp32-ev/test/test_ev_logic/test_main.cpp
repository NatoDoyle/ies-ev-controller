/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - runs on the PC with "pio test -e native".

  Description:
  Unit tests (Unity framework) for lib/ev_logic. These check the car logic
  without the ESP32: debouncing, gears, joystick scaling, motor and brake
  duty, headlights, indicators, temperature and the transmit array line.

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  cd esp32-ev && pio test -e native
*/
#include <unity.h>                    // Unity test framework
#include <string.h>                   // strcmp, strchr
#include "debouncer.h"                // code under test
#include "drive.h"
#include "lights.h"
#include "sensors.h"
#include "vehicle.h"
#include "telemetry.h"

void setUp() {}                       // run before each test (nothing needed)
void tearDown() {}                    // run after each test (nothing needed)

static const int CENTRE = 1900;       // joystick centre used in the tests

// Count the comma separated tokens in a line.
static int countTokens(const char* s) {
  int n = 1;                          // one token even with no commas
  for (; *s; s++) if (*s == ',') n++; // one more per comma
  return n;
}

// ---------------------------------------------------------------- Debouncer

void test_debounce_bounce_gives_one_edge() {
  Debouncer d(30);                    // 30 ms settle
  int edges = 0;                      // count press edges
  bool bounce[] = {true, false, true, false, true};  // contact bounce
  uint32_t t = 0;                     // time
  for (bool b : bounce) { d.update(b, t); edges += d.pressEdge(); t += 2; }  // bounce every 2 ms
  for (int i = 0; i < 50; i++) { d.update(true, t); edges += d.pressEdge(); t++; }  // then held
  TEST_ASSERT_EQUAL(1, edges);        // exactly one press seen
  TEST_ASSERT_TRUE(d.pressed());      // and it is held
}

void test_debounce_hold_does_not_repeat() {
  Debouncer d(30);                    // 30 ms settle
  int edges = 0;                      // count press edges
  for (uint32_t t = 0; t < 5000; t += 5) { d.update(true, t); edges += d.pressEdge(); }  // held 5 s
  TEST_ASSERT_EQUAL(1, edges);        // old MSP430 code would repeat every 250 ms
}

void test_debounce_release_then_press_again() {
  Debouncer d(30);                    // 30 ms settle
  int edges = 0;                      // count press edges
  uint32_t t = 0;                     // time
  for (; t < 100; t += 5) { d.update(true, t); edges += d.pressEdge(); }   // press
  for (; t < 200; t += 5) { d.update(false, t); edges += d.pressEdge(); }  // release
  TEST_ASSERT_FALSE(d.pressed());     // released
  for (; t < 300; t += 5) { d.update(true, t); edges += d.pressEdge(); }   // press again
  TEST_ASSERT_EQUAL(2, edges);        // two separate presses
}

void test_debounce_short_glitch_ignored() {
  Debouncer d(30);                    // 30 ms settle
  int edges = 0;                      // count press edges
  d.update(true, 0);  edges += d.pressEdge();       // glitch high
  d.update(true, 10); edges += d.pressEdge();       // still high, only 10 ms
  TEST_ASSERT_FALSE(d.pressed());     // not accepted yet (a pass-through would fail here)
  d.update(false, 20); edges += d.pressEdge();      // gone again
  for (uint32_t t = 20; t < 200; t += 5) { d.update(false, t); edges += d.pressEdge(); }  // stays low
  TEST_ASSERT_FALSE(d.pressed());     // glitch never accepted
  TEST_ASSERT_EQUAL(0, edges);        // and never reported as a press
}

// ---------------------------------------------------------------- Gears

void test_gear_up_clamps_at_top_no_wrap() {
  int g = GEAR_N;                     // start in N
  for (int i = 0; i < 10; i++) g = shiftGear(g, true, false);  // shift up 10 times
  TEST_ASSERT_EQUAL(4, g);            // stays at 4 (old code wrapped to 0)
}

void test_gear_down_clamps_at_neutral() {
  int g = 2;                          // in 2nd
  for (int i = 0; i < 5; i++) g = shiftGear(g, false, true);   // shift down 5 times
  TEST_ASSERT_EQUAL(GEAR_N, g);       // stays at N
}

// ---------------------------------------------------------------- Joystick

static AxisCal cal(int centre, bool invert = false) {  // helper: a calibration
  return calibrateAxis(centre, CENTRE, 1200, 2800, 150, invert);
}

void test_axis_deadzone_is_zero() {
  AxisCal c = cal(CENTRE);            // centred at 1900
  TEST_ASSERT_EQUAL(0, axisPercent(CENTRE, c));        // centre
  TEST_ASSERT_EQUAL(0, axisPercent(CENTRE + 150, c));  // edge of deadzone
  TEST_ASSERT_EQUAL(0, axisPercent(CENTRE - 150, c));  // other edge
}

void test_axis_extremes_reach_100() {
  AxisCal c = cal(CENTRE);            // centred at 1900
  TEST_ASSERT_EQUAL(100, axisPercent(4095, c));   // full up
  TEST_ASSERT_EQUAL(-100, axisPercent(0, c));     // full down
}

void test_axis_off_centre_both_ends_reach_100() {
  AxisCal c = cal(2600);              // stick that rests high
  TEST_ASSERT_EQUAL(100, axisPercent(4095, c));   // still reaches +100
  TEST_ASSERT_EQUAL(-100, axisPercent(0, c));     // still reaches -100
  int half = axisPercent(2600 + 150 + (4095 - 2600 - 150) / 2, c);  // half way up
  TEST_ASSERT_INT_WITHIN(1, 50, half);            // about 50 %
}

void test_axis_invert() {
  AxisCal c = cal(CENTRE, true);      // inverted
  TEST_ASSERT_EQUAL(-100, axisPercent(4095, c));  // up now reads negative
  TEST_ASSERT_EQUAL(100, axisPercent(0, c));      // down now reads positive
}

void test_default_cal_window_rejects_deflected_stick() {
  VehicleConfig cfg;                  // default settings
  VehicleState st;                    // the car
  vehicleInit(st, cfg, 1250, 2750);   // stick partly pushed down / right at power up
  TEST_ASSERT_TRUE(st.verCal.calFailed);   // rejected
  TEST_ASSERT_TRUE(st.horCal.calFailed);   // rejected
  Inputs rest; rest.ver = cfg.jsCentreDefault; rest.hor = cfg.jsCentreDefault;  // stick let go
  vehicleUpdate(st, rest, 0);         // one step
  TEST_ASSERT_EQUAL(0, st.thrPct);    // rest is not read as throttle
  TEST_ASSERT_EQUAL(0, st.brkPct);    // or as brake
}

void test_axis_calibration_fallback() {
  AxisCal c = cal(4000);              // stick held at power up
  TEST_ASSERT_TRUE(c.calFailed);      // flagged
  TEST_ASSERT_EQUAL(CENTRE, c.centre);  // default centre used
  AxisCal ok = cal(2000);             // normal reading
  TEST_ASSERT_FALSE(ok.calFailed);    // accepted
  TEST_ASSERT_EQUAL(2000, ok.centre); // measured centre used
}

// ---------------------------------------------------------------- Motor / brake

static const uint8_t GEARS[] = {0, 64, 128, 191, 255};  // N,1,2,3,4 caps

void test_motor_neutral_and_zero_throttle_off() {
  TEST_ASSERT_EQUAL(0, motorDuty(GEAR_N, 100, GEARS, 40));  // N = off
  TEST_ASSERT_EQUAL(0, motorDuty(3, 0, GEARS, 40));         // no throttle = off
}

void test_motor_gear_caps() {
  TEST_ASSERT_EQUAL(64, motorDuty(1, 100, GEARS, 40));   // 25 %
  TEST_ASSERT_EQUAL(128, motorDuty(2, 100, GEARS, 40));  // 50 %
  TEST_ASSERT_EQUAL(191, motorDuty(3, 100, GEARS, 40));  // 75 %
  TEST_ASSERT_EQUAL(255, motorDuty(4, 100, GEARS, 40));  // 100 %
}

void test_motor_min_duty_and_scaling() {
  TEST_ASSERT_EQUAL(42, motorDuty(4, 1, GEARS, 40));     // just above min at 1 %
  TEST_ASSERT_EQUAL(40 + (255 - 40) / 2, motorDuty(4, 50, GEARS, 40));  // half way
  TEST_ASSERT_EQUAL(64, motorDuty(1, 1, GEARS, 80));     // min above cap -> cap
}

void test_brake_duty() {
  TEST_ASSERT_EQUAL(0, brakeDuty(0, false, 25));     // no brake, no lights
  TEST_ASSERT_EQUAL(25, brakeDuty(0, true, 25));     // tail light level
  TEST_ASSERT_EQUAL(127, brakeDuty(50, false, 25));  // half brake
  TEST_ASSERT_EQUAL(255, brakeDuty(100, true, 25));  // full brake
  TEST_ASSERT_EQUAL(25, brakeDuty(5, true, 25));     // light brake not dimmer than tail
}

// ---------------------------------------------------------------- Headlight

void test_headlight_hysteresis_and_hold() {
  Headlight h(2600, 2200, 1000);      // on > 2600, off < 2200, 1 s hold
  TEST_ASSERT_FALSE(h.update(3000, 0));     // dark, timer starts
  TEST_ASSERT_FALSE(h.update(3000, 999));   // not held long enough
  TEST_ASSERT_TRUE(h.update(3000, 1000));   // held 1 s -> on
  TEST_ASSERT_TRUE(h.update(2400, 5000));   // in the band -> stays on
  TEST_ASSERT_TRUE(h.update(2000, 6000));   // bright, timer starts
  TEST_ASSERT_FALSE(h.update(2000, 7000));  // held 1 s -> off
  TEST_ASSERT_FALSE(h.update(2400, 9000));  // in the band -> stays off
}

void test_headlight_no_chatter() {
  Headlight h(2600, 2200, 1000);      // same settings
  for (uint32_t t = 0; t < 10000; t += 100) {        // 10 s of flicker
    bool lit = h.update((t / 100) % 2 ? 3000 : 2000, t);  // dark/bright every 100 ms
    TEST_ASSERT_FALSE(lit);           // never long enough to switch on
  }
}

// ---------------------------------------------------------------- Indicators

void test_indicator_toggle_switch_cancel() {
  Indicators ind(333);                // 333 ms half period
  ind.pressLeft(0);                   // S4
  TEST_ASSERT_TRUE(ind.leftActive());       // left on
  TEST_ASSERT_TRUE(ind.leftLamp());         // lamp lights at once
  ind.pressRight(100);                // S2 while left running
  TEST_ASSERT_FALSE(ind.leftActive());      // left off
  TEST_ASSERT_TRUE(ind.rightActive());      // switched to right
  ind.pressRight(200);                // S2 again
  TEST_ASSERT_EQUAL(IND_OFF, ind.side());   // cancelled
  TEST_ASSERT_FALSE(ind.rightLamp());       // lamp off
}

void test_indicator_blink_timing() {
  Indicators ind(333);                // 333 ms half period
  ind.pressLeft(1000);                // start at t = 1000
  ind.update(1000); TEST_ASSERT_TRUE(ind.leftLamp());    // 0 ms: on
  ind.update(1332); TEST_ASSERT_TRUE(ind.leftLamp());    // 332 ms: still on
  ind.update(1333); TEST_ASSERT_FALSE(ind.leftLamp());   // 333 ms: off
  ind.update(1666); TEST_ASSERT_TRUE(ind.leftLamp());    // 666 ms: on again
  TEST_ASSERT_FALSE(ind.rightLamp()); // right never lit
}

void test_indicator_millis_wrap() {
  Indicators ind(333);                // 333 ms half period
  uint32_t start = 0xFFFFFF00u;       // 256 ms before millis() wraps to 0
  ind.pressRight(start);              // start blinking
  ind.update(start + 300);            // 300 ms in (after the wrap)
  TEST_ASSERT_TRUE(ind.rightLamp());  // still in the first on half
  ind.update(start + 400);            // 400 ms in
  TEST_ASSERT_FALSE(ind.rightLamp()); // off half
}

// ---------------------------------------------------------------- Temperature

void test_temperature() {
  TEST_ASSERT_EQUAL(250, tempDeciC(250, false));  // LM35 250 mV = 25.0 C
  TEST_ASSERT_EQUAL(250, tempDeciC(750, true));   // TMP36 750 mV = 25.0 C
  TEST_ASSERT_TRUE(overheat(400, 400));           // at limit
  TEST_ASSERT_FALSE(overheat(399, 400));          // under limit
  TEST_ASSERT_FALSE(overheat(TEMP_NA, 400));      // no sensor
}

// ---------------------------------------------------------------- Telemetry

void test_telemetry_exact_line() {
  int16_t tx[TX_COUNT];               // transmit array
  for (int i = 0; i < TX_COUNT; i++) tx[i] = (int16_t)i;  // 0,1,2...
  tx[TX_TEMP] = TEMP_NA;              // include a negative value
  char buf[TX_LINE_MAX];              // line buffer
  int n = formatTxLine(tx, buf, sizeof buf);  // build the line
  TEST_ASSERT_EQUAL_STRING("$EV,0,1,2,3,4,5,6,7,-999,9,10,11,12,13,14,15,16,17,18,19,20\n", buf);
  TEST_ASSERT_EQUAL((int)strlen(buf), n);     // length returned
  TEST_ASSERT_EQUAL(TX_COUNT + 1, countTokens(buf));  // "$EV" + 21 values
}

void test_telemetry_worst_case_fits_and_small_buffer_fails() {
  int16_t tx[TX_COUNT];               // transmit array
  for (int i = 0; i < TX_COUNT; i++) tx[i] = -32768;      // longest numbers
  char buf[TX_LINE_MAX];              // normal buffer
  TEST_ASSERT_TRUE(formatTxLine(tx, buf, sizeof buf) > 0); // fits
  char tiny[20];                      // too small
  TEST_ASSERT_EQUAL(-1, formatTxLine(tx, tiny, sizeof tiny));  // refused, no overflow
}

void test_telemetry_header_matches_fields() {
  const char* h = txHeader();         // header line
  TEST_ASSERT_EQUAL('#', h[0]);       // starts with #
  TEST_ASSERT_EQUAL(TX_COUNT + 1, countTokens(h));  // "#EV" + 21 names
  TEST_ASSERT_EQUAL('\n', h[strlen(h) - 1]);        // ends in newline
}

// ---------------------------------------------------------------- Whole car

// Helper: run the car for ms milliseconds in 10 ms steps with fixed inputs.
static uint32_t run(VehicleState& st, Inputs in, uint32_t t, uint32_t ms) {
  for (uint32_t end = t + ms; t < end; t += 10) vehicleUpdate(st, in, t);  // step
  return t;                           // new time
}

// Helper: press and release one switch.
static uint32_t tap(VehicleState& st, Inputs base, bool Inputs::*sw, uint32_t t) {
  Inputs down = base;                 // copy of the inputs
  down.*sw = true;                    // press the chosen switch
  t = run(st, down, t, 100);          // hold 100 ms
  return run(st, base, t, 100);       // release 100 ms
}

void test_vehicle_end_to_end() {
  VehicleConfig cfg;                  // default settings
  VehicleState st;                    // the car
  vehicleInit(st, cfg, CENTRE, CENTRE);  // calibrate at centre
  Inputs idle;                        // nothing pressed
  idle.ver = CENTRE; idle.hor = CENTRE; idle.ldr = 1000;  // centred, daylight
  uint32_t t = run(st, idle, 0, 100); // settle
  TEST_ASSERT_FALSE(st.ignition);     // starts off

  t = tap(st, idle, &Inputs::s1, t);  // S1 with ignition off
  TEST_ASSERT_EQUAL(GEAR_N, st.gear); // no gear change while off

  t = tap(st, idle, &Inputs::js, t);  // joystick button
  TEST_ASSERT_TRUE(st.ignition);      // started
  t = tap(st, idle, &Inputs::s1, t);  // up to 1
  t = tap(st, idle, &Inputs::s1, t);  // up to 2
  TEST_ASSERT_EQUAL(2, st.gear);      // in 2nd

  Inputs full = idle; full.ver = 4095;  // full throttle
  t = run(st, full, t, 50);           // drive
  TEST_ASSERT_EQUAL(100, st.thrPct);  // full throttle read
  TEST_ASSERT_EQUAL(128, st.motor);   // gear 2 cap
  TEST_ASSERT_EQUAL(0, st.brake);     // no brake light

  Inputs brake = idle; brake.ver = 0; // full brake
  t = run(st, brake, t, 50);          // brake
  TEST_ASSERT_EQUAL(0, st.motor);     // motor off while braking
  TEST_ASSERT_EQUAL(255, st.brake);   // brake light full

  t = tap(st, idle, &Inputs::js, t);  // stop
  TEST_ASSERT_FALSE(st.ignition);     // off
  TEST_ASSERT_EQUAL(GEAR_N, st.gear); // back to N
  t = run(st, full, t, 50);           // throttle with ignition off
  TEST_ASSERT_EQUAL(0, st.motor);     // motor stays off
}

void test_vehicle_throttle_interlock() {
  VehicleConfig cfg;                  // default settings
  VehicleState st;                    // the car
  vehicleInit(st, cfg, CENTRE, CENTRE);  // calibrate
  Inputs idle; idle.ver = CENTRE; idle.hor = CENTRE;  // centred
  uint32_t t = run(st, idle, 0, 100); // settle
  t = tap(st, idle, &Inputs::js, t);  // ignition on
  t = tap(st, idle, &Inputs::s1, t);  // gear 1

  Inputs full = idle; full.ver = 4095;  // full throttle
  t = tap(st, idle, &Inputs::js, t);  // ignition off
  Inputs pressFull = full; pressFull.js = true;  // turn on WITH throttle held
  t = run(st, pressFull, t, 100);     // press button while throttle held
  t = run(st, full, t, 100);          // release button, throttle still held
  TEST_ASSERT_TRUE(st.ignition);      // running
  t = tap(st, full, &Inputs::s1, t);  // shift into gear 1 with the throttle STILL held
  TEST_ASSERT_EQUAL(1, st.gear);      // in gear
  TEST_ASSERT_EQUAL(0, st.motor);     // but motor blocked until throttle released

  t = run(st, idle, t, 50);           // let go of the throttle (arms the motor)
  t = run(st, full, t, 50);           // now throttle
  TEST_ASSERT_EQUAL(64, st.motor);    // motor runs (gear 1 cap)
}

void test_vehicle_auto_headlights_and_tail_light() {
  VehicleConfig cfg;                  // default settings
  VehicleState st;                    // the car
  vehicleInit(st, cfg, CENTRE, CENTRE);  // calibrate
  Inputs dark; dark.ver = CENTRE; dark.hor = CENTRE; dark.ldr = 3500;  // dark
  uint32_t t = run(st, dark, 0, 1200);  // over 1 s of darkness
  TEST_ASSERT_TRUE(st.headOn);        // headlights came on automatically
  TEST_ASSERT_EQUAL(cfg.tailDuty, st.brake);  // tail light glow
  (void)t;                            // time not needed after this
}

void test_vehicle_temp_disabled_sends_na() {
  VehicleConfig cfg;                  // default: temperature disabled
  VehicleState st;                    // the car
  vehicleInit(st, cfg, CENTRE, CENTRE);  // calibrate
  Inputs in; in.ver = CENTRE; in.hor = CENTRE; in.tempMv = 900;  // pin floating
  vehicleUpdate(st, in, 0);           // one step
  TEST_ASSERT_EQUAL(TEMP_NA, st.tempDeciC);  // reported as not available
  TEST_ASSERT_FALSE(st.hot);          // no false overheat
}

void test_vehicle_tx_array_from_state() {
  VehicleConfig cfg;                  // default settings
  VehicleState st;                    // the car
  vehicleInit(st, cfg, CENTRE, CENTRE);  // calibrate
  Inputs in; in.ver = CENTRE; in.hor = 4095; in.ldr = 1234; in.s4 = true;  // steer right, hold S4
  run(st, in, 0, 100);                // settle
  int16_t tx[TX_COUNT];               // transmit array
  buildTxArray(st, 12345, tx);        // fill it
  TEST_ASSERT_EQUAL(2345, tx[TX_SEQ]);     // wraps at 9999
  TEST_ASSERT_EQUAL(100, tx[TX_STEER]);    // full right
  TEST_ASSERT_EQUAL(1234, tx[TX_LDR]);     // LDR copied
  TEST_ASSERT_EQUAL(1, tx[TX_S4]);         // S4 held
  TEST_ASSERT_EQUAL(1, tx[TX_IND_L]);      // left indicator on
  TEST_ASSERT_EQUAL(0, tx[TX_S1]);         // S1 not pressed
  TEST_ASSERT_EQUAL(TEMP_NA, tx[TX_TEMP]); // no sensor
}

int main() {
  UNITY_BEGIN();                      // start the test run
  RUN_TEST(test_debounce_bounce_gives_one_edge);
  RUN_TEST(test_debounce_hold_does_not_repeat);
  RUN_TEST(test_debounce_release_then_press_again);
  RUN_TEST(test_debounce_short_glitch_ignored);
  RUN_TEST(test_gear_up_clamps_at_top_no_wrap);
  RUN_TEST(test_gear_down_clamps_at_neutral);
  RUN_TEST(test_axis_deadzone_is_zero);
  RUN_TEST(test_axis_extremes_reach_100);
  RUN_TEST(test_axis_off_centre_both_ends_reach_100);
  RUN_TEST(test_axis_invert);
  RUN_TEST(test_axis_calibration_fallback);
  RUN_TEST(test_default_cal_window_rejects_deflected_stick);
  RUN_TEST(test_motor_neutral_and_zero_throttle_off);
  RUN_TEST(test_motor_gear_caps);
  RUN_TEST(test_motor_min_duty_and_scaling);
  RUN_TEST(test_brake_duty);
  RUN_TEST(test_headlight_hysteresis_and_hold);
  RUN_TEST(test_headlight_no_chatter);
  RUN_TEST(test_indicator_toggle_switch_cancel);
  RUN_TEST(test_indicator_blink_timing);
  RUN_TEST(test_indicator_millis_wrap);
  RUN_TEST(test_temperature);
  RUN_TEST(test_telemetry_exact_line);
  RUN_TEST(test_telemetry_worst_case_fits_and_small_buffer_fails);
  RUN_TEST(test_telemetry_header_matches_fields);
  RUN_TEST(test_vehicle_end_to_end);
  RUN_TEST(test_vehicle_throttle_interlock);
  RUN_TEST(test_vehicle_auto_headlights_and_tail_light);
  RUN_TEST(test_vehicle_temp_disabled_sends_na);
  RUN_TEST(test_vehicle_tx_array_from_state);
  return UNITY_END();                 // report results
}

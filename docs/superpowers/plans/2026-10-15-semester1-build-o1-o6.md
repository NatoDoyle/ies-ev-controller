# Semester 1 Build (O1–O6) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Get the existing EV controller running on real hardware, then add the DC servo rig speed sensor, TIP121 motor drive and motor-current sensing to the firmware and the Processing dashboard, with the circuits, test procedures, lab checklists and A3 poster needed to hit 22 Oct and 5 Nov.

**Architecture:** New logic goes in `lib/ev_logic` as pure C++ (no `Arduino.h`), unit-tested on the Mac with `pio test -e native`, the same as the existing code. The only new hardware code is a small interrupt module (`src/speed_input.*`) that counts disc pulses. The loop turns those counts into rpm every 100 ms, reads the current-sense pin, and passes both through `Inputs` → `VehicleState` → the transmit array (two fields appended: `rpm`, `imA`) → both dashboards.

**Tech Stack:** ESP32-WROVER-E DevKitC-VE · PlatformIO 6.2 with the pinned pioarduino platform 55.03.312 (arduino-esp32 3.3.12) · Unity tests (native env) · Processing 4.5.7 (`processing.serial`) · plain HTML/JS web dashboard · Google Chrome headless (poster PDF).

**Spec:** `docs/superpowers/specs/2026-10-15-semester1-outcomes-design.md`

All paths below are relative to the repo root `IES/` unless they start with `esp32-ev/` (which is also inside `IES/`).

## Global Constraints

- Platform stays `pioarduino 55.03.312-1` (arduino-esp32 core 3.3.12); do not switch to the official espressif32 platform.
- `lib/ev_logic` stays pure C++: no `Arduino.h`, no pins, integer maths only, builds in `pio test -e native`.
- Analog inputs use ADC1 only (GPIO32–39), because ADC2 stops working while WiFi is on.
- Pins to avoid: 0, 2, 5, 12, 15 (strapping), 6–11 (flash), 16–17 (PSRAM), 1/3 (USB serial). GPIO14 is allowed only as the optional home-test output.
- Every edited source file keeps the housekeeping header (`Written / by / Student No. / Revision / Hardware / Description / History / Configuration`). Bump `Revision` and add a `History` line dated 20261015. Keep the trailing line comments on every code line (IES outcome 10: "fully comment all code").
- The transmit array is append-only. When a field is added, update `TX_COUNT`, `TxField`, `txHeader()`, `TX_LINE_MAX`, `esp32-ev/web/index.html` (`FIELDS` + demo line) and `dashboard/EV_Dashboard/EV_Dashboard.pde` (`NUM_FIELDS`, indexes, demo) in the same task.
- Processing is the graded dashboard. The web dashboard must keep parsing every line; showing the new fields there is a stretch goal and not part of this plan.
- Rig facts: the speed output is a 0–5 V square wave; the motor is 24 V DC; the armature current output is 1 V/A (maximum voltage not yet measured); the motor is driven by the student's own TIP121 stage.
- Learning bar: the student must be able to explain every line. Every code task ends with an **explain-it checkpoint**. The controller asks the student those questions and waits for their answers before starting the next task. If an answer is shaky, the controller walks through the code with them. It never answers for them.
- Commits use the repo identity already configured (Nathan Doyle, noreply address). Every commit message ends with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Lecturer PDFs never get committed (`*.pdf` is ignored; the poster PDF is force-added).
- Before ending every task: `cd esp32-ev && pio test -e native` is green. From Task 5 on, `pio run -e esp32` also builds.
- Firmware and dashboard reach `main` together: Task 5 commits but does **not** push; the push happens at the end of Task 6.
- Scope: O0 is done (repo public, initial state `6f54d9f`). This plan covers O1–O6, starts O11 and builds the O2 poster. O7–O10 and the rest of O11 get a separate plan after 5 Nov.
- Rig facts that are **not** confirmed (rated rpm, gearbox ratio, which shaft the disc is on) are written as "to confirm" everywhere until the manual or technician confirms them.

## Review Focus

1. **Rig off or speed wire unplugged.** rpm must read 0 and stay 0, never a frozen last value. Owned by Task 2: `test_speed_no_signal_reads_zero` and `test_speed_stops_slows_and_restarts`.
2. **PWM motor switching couples noise onto the speed wire.** Edges closer than `SPEED_GLITCH_US` are ignored. Owned by Task 2: `test_speed_glitch_filter`. Home pre-check: Task 7 home test C step 5 (test wave next to the driven kit motor). Rig check: the 5 Nov checklist step "rpm steady while TIP121 drives".
3. **Slots-per-rev not set (0) or a very high frequency.** rpm returns 0, or saturates at 32767, and never goes negative or divides by zero. Owned by Task 2: `test_rpm_from_frequency`.
4. **Current input at 0 A, over-range, or set-up value 0.** The ESP32 ADC never reads below about 142 mV, so readings under `IMOTOR_FLOOR_MV` (150 mV) must show 0 A, not 0.28 A. Over-range saturates at 32767 mA, never negative, never divides by zero. Owned by Task 3: `test_motor_current_conversion`.
5. **Firmware and dashboard disagree on the field count.** Lines are silently dropped and the dashboard freezes. Processing shows a `BAD LINES n` counter (Task 6), exercised against the old 21-field firmware in H2 step 4. The web parser is checked against a 23-field line, and rejects a 21-field line, by a Node script (Task 4).

---

### Task 1: Bring-up checklist and logbook skeleton (unblocks O1 today)

**Files:**
- Create: `docs/bring-up-checklist.md`
- Create: `docs/logbook.md`

**Interfaces:**
- Consumes: the firmware as committed (`esp32-ev/include/pins.h`, `config.h`, transmit array in `telemetry.h`).
- Produces: the O1 checklist the student runs on the hardware (Task H1), and the logbook file every later task's evidence goes into.

- [ ] **Step 1: Write `docs/bring-up-checklist.md`**

```markdown
# O1 bring-up checklist: existing EV controller on the real WROVER

Goal (spec O1, due 21 Oct): every existing feature verified on the wired board.
Run it top to bottom. Write the result of every row in the table at the end and copy
the table into the logbook. If a row fails, stop and note what you saw. Fix it (or ask
Claude, quoting the row number) before moving on.

## 0. Before power

| # | Check | Expected |
|---|---|---|
| 0.1 | Every wire against the pin map in `esp32-ev/README.md` | Matches. Switches go between the pin and GND |
| 0.2 | Joystick VCC on **3V3**, not 5V | 3V3 (5 V on GPIO34/39 can damage the ESP32) |
| 0.3 | LM35: +Vs on 5V, Vout on GPIO35, GND | LM35 output is about 10 mV/°C, well under 3.3 V |
| 0.4 | TIP120: base through 1 kΩ from GPIO23, emitter to GND, collector to motor −; motor + to 5V; diode across the motor, stripe (cathode) to 5V | As described |
| 0.5 | 7-segment: a = GPIO22, b+c = GPIO21, d = GPIO18, e+f = GPIO19, each through its resistor; common pin to GND (common cathode) | As described |

## 1. Flash and serial

| # | Do | Expected |
|---|---|---|
| 1.1 | `cd esp32-ev && pio run -t upload` | `SUCCESS`. If no port: hold BOOT, tap EN, release BOOT, retry |
| 1.2 | `pio device monitor`, then press EN on the board | `# Web dashboard: join WiFi "EV-Dashboard" and open http://192.168.4.1`, then the `#EV,seq,ign,...` header, then `$EV,...` lines 10 times a second |
| 1.3 | Type `?` | The `#EV,...` header prints again |

Leave the joystick untouched while the board powers up: it measures its centre at boot.
If you see `# Joystick centre out of range - using default`, power-cycle without touching it.

## 2. Inputs (watch the `$EV` fields: thr=3, brk=4, steer=5, ldr=6, tempdC=8, s1..js=16..20)

| # | Do | Expected |
|---|---|---|
| 2.1 | Joystick at rest | thr 0, brk 0, steer 0 |
| 2.2 | Push fully up / fully down | thr → 100 / brk → 100. If they're swapped, set `THROTTLE_INVERT true` in `config.h` |
| 2.3 | Push fully left / right | steer → −100 / +100. If backwards, set `STEER_INVERT true` |
| 2.4 | Press S1, S2, S3, S4, joystick button one at a time | Matching field (s1 s2 s3 s4 js) is 1 only while held |
| 2.5 | Room light, then cover the LDR with a finger | Note both ldr values (light ___, dark ___). Dark must go above 2600 |
| 2.6 | Temperature at rest, then pinch the LM35 | tempdC ≈ 200–260 (20–26 °C), then rises |

## 3. Car behaviour

| # | Do | Expected |
|---|---|---|
| 3.1 | Press joystick button | ign 0 → 1. Press again → 0. Leave it on (1) |
| 3.2 | S1 four times, then once more | gear 1, 2, 3, 4, stays 4 (no wrap) |
| 3.3 | S3 five times | gear 3, 2, 1, 0, stays 0 |
| 3.4 | S4 | indL 1, lampL flashes ~1.5 Hz, segments e+f flash |
| 3.5 | S2 | Switches to right: indR 1, b+c flash. S2 again cancels |
| 3.6 | Cover the LDR for 2 s, uncover for 2 s | head 1 about 1 s after covering (segment a on); head 0 about 1 s after uncovering |
| 3.7 | Gear 1, throttle up from the centre | motor 40 → 64 as the stick goes up; the motor turns |
| 3.8 | Gear 4, full throttle | motor 255, full speed |
| 3.9 | Ignition off. Hold the stick fully up, press the joystick button (ign → 1), and still holding up press S1 (gear 1) | motor stays 0 (throttle interlock). Let the stick go back to the centre, then push up again: motor 40 → 64 |
| 3.10 | Brake (stick down) | motor 0, brake 255 at full brake, segment d bright |
| 3.11 | Headlights on (LDR covered), no brake | brake 25 (dim tail light on segment d) |

## 4. Dashboards

| # | Do | Expected |
|---|---|---|
| 4.1 | Close the serial monitor. Open `dashboard/EV_Dashboard/EV_Dashboard.pde` in Processing, Run | Status bar `PORT /dev/cu.… @ 115200` in green, values move with the joystick. P = next port if the wrong one opened |
| 4.2 | Press H | The Processing console prints `Header: #EV,...` |
| 4.3 | Phone/laptop: join WiFi `EV-Dashboard` (password in `config.h`), open http://192.168.4.1 | Web dashboard updates live |

## Results (copy into the logbook)

| Row | Pass/Fail | Notes (values seen, what you changed) |
|---|---|---|
| 0.1–0.5 | | |
| 1.1–1.3 | | |
| 2.1 | | |
| 2.2 | | |
| 2.3 | | |
| 2.4 | | |
| 2.5 | | light ___ dark ___ |
| 2.6 | | ___ °C |
| 3.1–3.3 | | |
| 3.4–3.5 | | |
| 3.6 | | |
| 3.7–3.9 | | |
| 3.10–3.11 | | |
| 4.1–4.3 | | |
```

- [ ] **Step 2: Write `docs/logbook.md`**

```markdown
# Logbook: EV Controller Interface (IES)

Nathan Doyle · Interactive Embedded Systems · TU Dublin · 2026/27

How this logbook works:
- One entry per working session, newest at the bottom, dated. Each commit records when
  an entry was written.
- Written in my own words. Diagrams go in `docs/logbook/` and are linked from the entry.
- Export: print this page to PDF (VS Code "Markdown: Open Preview" → print, or open
  it on GitHub and print). Print only if a paper copy is asked for.

## Retrospective (written 15 Oct 2026, covering the weeks before this logbook started)

These notes were written after the fact, from memory, the lecture slides and the code,
not on the days they describe.

- Weeks 1–5, MSP430 sessions (LEDs, switches, ADC, temperature, LDR, PWM, 7-segment,
  DC motor, Processing):
- First version of the EV controller on the MSP430 (`firmware.cpp`):
- 8 Oct: port to the ESP32-WROVER-E, and why (pins, ADC1 vs ADC2, WiFi dashboard):
- Paper design: system block diagram and main loop flowchart (see `esp32-ev/README.md`
  "Flowcharts" and copy/redraw them here):

## Entries

### 15 Oct 2026: real project scope and plan

- Aim:
- What I did:
- Results / measurements:
- Problems and fixes:
- Design work (diagrams, calculations):
- Next:

<!-- Template for each new entry:
### DD Mon YYYY: title
- Aim:
- What I did:
- Results / measurements:
- Problems and fixes:
- Design work (diagrams, calculations):
- Next:
-->
```

- [ ] **Step 3: Commit, tag the baseline firmware, push**

```bash
git add docs/bring-up-checklist.md docs/logbook.md docs/superpowers/plans/2026-10-15-semester1-build-o1-o6.md
git commit -m "Add O1 bring-up checklist, logbook skeleton and the O1-O6 build plan

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git tag o1-baseline
git push && git push origin o1-baseline
```

`o1-baseline` marks the firmware exactly as it was before any Semester 1 change (21 fields). H1 flashes it, and H2 step 4 uses it to prove the `BAD LINES` warning.

- [ ] **Step 4: Tell the student H0 and H1 can start now.** H1 flashes the tag, not whatever is on `main` that day: `git checkout o1-baseline`, flash, `git checkout main` afterwards.

---

### Task H0 (student): ask before you build, by Fri 16 Oct

**Executor: the student. Not a subagent task.**

- [ ] Email or ask the lab technician for the Feedback 33-100 manual, and these answers: which socket is the speed output; slots per revolution on the disc and which shaft it is on; gearbox ratio and what the rig's display shows (rpm or volts); the current output's range and polarity; how to reach the motor terminals; whether an external motor drive is allowed on 5 Nov (and if not, the amplifier input's voltage range).
- [ ] Ask the lecturer: the poster date (22 Oct or 5 Nov), and whether the module needs a declaration of AI assistance on submitted work.
- [ ] Log both sets of answers in the logbook. Anything still unanswered gets asked at the rig on 22 Oct (lab step 1).

---

### Task H1 (student, hardware): run the O1 bring-up, by Wed 21 Oct

**Executor: the student, at home. Not a subagent task.**

- [ ] `git checkout o1-baseline`, flash, and run `docs/bring-up-checklist.md` top to bottom on the wired WROVER. Afterwards `git checkout main`.
- [ ] Fill in the results table, and write a logbook entry with it and any changes (e.g. `THROTTLE_INVERT`, LDR thresholds).
- [ ] Every fail goes back to Claude with the row number and what was seen (debug with superpowers:systematic-debugging before changing code).
- [ ] Commit config changes and the logbook entry. Config fixes found here (e.g. `THROTTLE_INVERT`) are made on `main`, not on the tag.
- [ ] **Explain-it checkpoint (spec O1):** the controller asks the student to trace one press of S1 from the pin to the screen: GPIO25 → `readInputs()` → `Debouncer` → `shiftGear()` → `buildTxArray()` (`TX_S1`, `TX_GEAR`) → `formatTxLine()` → USB → `serialEvent()` → `drawGear()`. The student answers; the controller only corrects.

---

### Task 2: Speed estimator in `lib/ev_logic` (O3 logic)

Port the tested estimator from `fs-master-control-unit/lib/mcu_core/src/speed.{h,cpp}` (same author), rewritten in this project's style. Leave out the storm guard and the km/h maths (YAGNI). Add an rpm conversion.

**Files:**
- Create: `esp32-ev/lib/ev_logic/src/speed.h`
- Create: `esp32-ev/lib/ev_logic/src/speed.cpp`
- Modify: `esp32-ev/test/test_ev_logic/test_main.cpp` (new tests before `int main()`, `RUN_TEST` lines, `#include "speed.h"`)

**Interfaces:**
- Produces (used by Task 5):
  - `inline bool acceptEdge(uint32_t nowUs, bool hasLast, uint32_t lastAcceptedUs, uint32_t glitchUs)`
  - `struct EdgeSnapshot { uint32_t count; uint32_t lastEdgeUs; };`
  - `class SpeedEstimator { public: SpeedEstimator(uint32_t stopTimeoutUs = 500000); void update(const EdgeSnapshot& snap, uint32_t nowUs); uint32_t hzX10() const; bool stopped() const; };`
  - `int16_t rpmFromHzX10(uint32_t hzX10, uint16_t pulsesPerRev);`

- [ ] **Step 1: Write the failing tests.** In the `test_main.cpp` header change `Revision:       1.0` to `Revision:       1.1`, change the line `  duty, headlights, indicators, temperature and the transmit array line.` to the two lines `  duty, headlights, indicators, temperature, the transmit array line, the rig` and `  speed estimator and motor current.`, and after the lines `  V1.0 :- 20261008` / `  First version.` add `  V1.1 :- 20261015` / `  Speed, motor current and rig transmit-array tests.` (Tasks 3 and 4 then need no further header change). Add `#include "speed.h"` after `#include "telemetry.h"`. Insert this block, followed by one blank line, immediately before the line `int main() {`:

```cpp
// ---------------------------------------------------------------- Speed sensor

// Pretend interrupt: counts edges exactly like src/speed_input.cpp does.
struct FakeIsr {
  uint32_t count = 0;                 // accepted edges
  uint32_t lastUs = 0;                // time of the last accepted edge
  bool hasLast = false;               // has any edge been accepted yet?
  void edge(uint32_t us) {            // one rising edge at time us
    if (!acceptEdge(us, hasLast, lastUs, 20)) return;  // closer than 20 us = noise
    count++;                          // count it
    lastUs = us;                      // remember when
    hasLast = true;                   // now there is a last edge
  }
  EdgeSnapshot snap() const { return EdgeSnapshot{count, lastUs}; }  // what main.cpp copies
};

// Deliver a steady square wave: edges every periodUs from nextUs up to untilUs.
// Returns the time of the first edge NOT delivered (where to carry on next time).
static uint64_t feed(FakeIsr& isr, uint64_t nextUs, uint64_t periodUs, uint64_t untilUs) {
  while (nextUs <= untilUs) {         // every edge that has happened by untilUs
    isr.edge((uint32_t)nextUs);       // the interrupt sees it
    nextUs += periodUs;               // next edge one period later
  }
  return nextUs;                      // carry on from here
}

void test_speed_glitch_filter() {
  TEST_ASSERT_TRUE(acceptEdge(1005, false, 1000, 20));      // first edge always counts
  TEST_ASSERT_FALSE(acceptEdge(1019, true, 1000, 20));      // 19 us after the last = noise
  TEST_ASSERT_TRUE(acceptEdge(1020, true, 1000, 20));       // 20 us = a real edge
  TEST_ASSERT_TRUE(acceptEdge(4, true, 0xFFFFFFF0u, 20));   // 20 us across the micros() wrap
  TEST_ASSERT_FALSE(acceptEdge(3, true, 0xFFFFFFF0u, 20));  // 19 us across the wrap
}

void test_speed_no_signal_reads_zero() {
  FakeIsr isr;                        // no edges ever (rig off or wire unplugged)
  SpeedEstimator est;                 // 0.5 s stop timeout
  for (uint32_t now = 100000; now <= 3000000; now += 100000) {  // 3 s of updates
    est.update(isr.snap(), now);      // nothing new
    TEST_ASSERT_TRUE(est.stopped());  // still stopped
    TEST_ASSERT_EQUAL_UINT32(0, est.hzX10());  // still 0 Hz
  }
}

void test_speed_100hz_steady() {
  FakeIsr isr;                        // pretend interrupt
  SpeedEstimator est;                 // estimator under test
  uint64_t next = feed(isr, 5000, 10000, 100000);  // 100 Hz = an edge every 10 000 us
  est.update(isr.snap(), 100000);     // first edges only set the reference
  TEST_ASSERT_TRUE(est.stopped());    // no speed yet
  TEST_ASSERT_EQUAL_UINT32(0, est.hzX10());
  for (uint32_t now = 200000; now <= 1000000; now += 100000) {  // every 100 ms
    next = feed(isr, next, 10000, now);   // 10 more edges
    est.update(isr.snap(), now);      // measure
    TEST_ASSERT_FALSE(est.stopped()); // turning
    TEST_ASSERT_EQUAL_UINT32(1000, est.hzX10());  // exactly 100.0 Hz
  }
}

void test_speed_2khz_with_jitter_within_1pct() {
  static const int jit[] = {20, -20, 15, -5, -20, 20, 0, -10};  // edge timing errors in us
  FakeIsr isr;                        // pretend interrupt
  SpeedEstimator est;                 // estimator under test
  uint64_t k = 0;                     // edge number
  for (uint32_t now = 100000; now <= 2000000; now += 100000) {  // 2 s, update every 100 ms
    while (true) {                    // deliver every edge up to now
      int64_t t = (int64_t)(k * 500 + 250) + jit[k % 8];  // 2 kHz = 500 us period, plus jitter
      if (t > (int64_t)now) break;    // not happened yet
      isr.edge((uint32_t)t);          // the interrupt sees it
      k++;                            // next edge
    }
    est.update(isr.snap(), now);      // measure
    if (now >= 200000) TEST_ASSERT_UINT32_WITHIN(200, 20000, est.hzX10());  // 2000.0 Hz +/- 1 %
  }
}

void test_speed_stops_slows_and_restarts() {
  FakeIsr isr;                        // pretend interrupt
  SpeedEstimator est;                 // 0.5 s stop timeout
  uint64_t next = feed(isr, 5000, 10000, 100000);  // 100 Hz
  est.update(isr.snap(), 100000);     // reference
  feed(isr, next, 10000, 200000);     // 10 more edges, last one at 195 000 us
  est.update(isr.snap(), 200000);     // measure
  TEST_ASSERT_EQUAL_UINT32(1000, est.hzX10());   // 100.0 Hz
  est.update(isr.snap(), 400000);     // edges stopped: 205 000 us since the last one
  TEST_ASSERT_EQUAL_UINT32(48, est.hzX10());     // capped at 10 000 000 / 205 000 = 48 (4.8 Hz)
  TEST_ASSERT_FALSE(est.stopped());   // slowing, not stopped yet
  est.update(isr.snap(), 695000);     // exactly 500 000 us since the last edge
  TEST_ASSERT_FALSE(est.stopped());   // not MORE than the timeout yet
  est.update(isr.snap(), 695001);     // 500 001 us
  TEST_ASSERT_TRUE(est.stopped());    // stopped
  TEST_ASSERT_EQUAL_UINT32(0, est.hzX10());      // reads 0
  isr.edge(800000);                   // disc starts again: first edge
  est.update(isr.snap(), 800100);     // one edge only sets a new reference
  TEST_ASSERT_TRUE(est.stopped());    // its period is unknown, so still 0
  TEST_ASSERT_EQUAL_UINT32(0, est.hzX10());
  isr.edge(810000);                   // second edge 10 000 us later
  est.update(isr.snap(), 810100);     // now it can measure
  TEST_ASSERT_EQUAL_UINT32(1000, est.hzX10());   // 100.0 Hz again
}

void test_speed_edge_counter_wrap() {
  SpeedEstimator est;                 // estimator under test
  est.update(EdgeSnapshot{0xFFFFFFFBu, 50000}, 60000);  // reference just before the counter wraps
  est.update(EdgeSnapshot{5u, 150000}, 160000);         // 10 edges later (wrapped past 0) in 100 000 us
  TEST_ASSERT_EQUAL_UINT32(1000, est.hzX10());          // 100.0 Hz, not nonsense
}

void test_rpm_from_frequency() {
  TEST_ASSERT_EQUAL(600, rpmFromHzX10(1000, 10));      // 100 Hz / 10 slots = 10 rev/s = 600 rpm
  TEST_ASSERT_EQUAL(3000, rpmFromHzX10(5000, 10));     // 500 Hz / 10 slots = 3000 rpm
  TEST_ASSERT_EQUAL(500, rpmFromHzX10(5000, 60));      // 60 "slots": rpm reads the same as Hz (home test)
  TEST_ASSERT_EQUAL(0, rpmFromHzX10(1000, 0));         // slots not set -> 0, no divide by zero
  TEST_ASSERT_EQUAL(32767, rpmFromHzX10(4000000000u, 1));  // too big -> saturates, never negative
}
```

Then add these lines at the end of the `RUN_TEST` list in `main()` (after `RUN_TEST(test_vehicle_tx_array_from_state);`):

```cpp
  RUN_TEST(test_speed_glitch_filter);
  RUN_TEST(test_speed_no_signal_reads_zero);
  RUN_TEST(test_speed_100hz_steady);
  RUN_TEST(test_speed_2khz_with_jitter_within_1pct);
  RUN_TEST(test_speed_stops_slows_and_restarts);
  RUN_TEST(test_speed_edge_counter_wrap);
  RUN_TEST(test_rpm_from_frequency);
```

- [ ] **Step 2: Run the tests and make sure they fail**

Run: `cd esp32-ev && pio test -e native`
Expected: build FAILS with `'speed.h' file not found` (clang, the Mac's compiler) or `speed.h: No such file or directory` (gcc).

- [ ] **Step 3: Write `esp32-ev/lib/ev_logic/src/speed.h`**

```cpp
/*
  Written         15/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Motor speed from the DC servo rig's slotted disc. Every slot gives one
  rising edge (a low-to-high step) of the rig's 0-5 V square wave.
   - Interrupt side (src/speed_input.cpp): on every edge, acceptEdge()
     throws away noise, then the edge is counted and timestamped.
   - Loop side (main.cpp, every 100 ms): the count and the time of the last
     edge (an EdgeSnapshot) go into SpeedEstimator::update(), which works
     out the frequency. rpmFromHzX10() turns that into rpm.
  Method: count over span. If n new edges arrived since the reference edge,
  and they took span microseconds, the frequency is n / span.
  Worked example: 10 edges in 100 000 us -> 10 * 10 000 000 / 100 000 = 1000
  tenths of a Hz = 100.0 Hz. With 10 slots: 100 / 10 * 60 = 600 rpm.
  "X10" means tenths (hzX10 = 1000 is 100.0 Hz): one decimal, no floats.
  Times are micros() values, which wrap to 0 every 71.6 minutes. Every time
  subtraction is unsigned, so it stays correct across the wrap.
  Ported from the FS Master Control Unit (fs-master-control-unit,
  lib/mcu_core/src/speed.h), same author. The storm guard and the km/h
  maths were left out because this project doesn't need them.

  History:
  V1.0 :- 20261015
  First version.

  Configuration:
  SPEED_GLITCH_US, SPEED_STOP_TIMEOUT_US and SPEED_PULSES_PER_REV in
  include/config.h.
*/
#ifndef EV_SPEED_H                    // include guard
#define EV_SPEED_H                    // mark as included

#include <stdint.h>                   // fixed width integer types

// Interrupt side: accept an edge unless it came less than glitchUs after the
// last accepted edge (noise). hasLast = false means no edge yet, so accept.
// "inline" lets the interrupt use it without a function call.
// Example (glitchUs 20, last edge at 1000 us): 1019 -> rejected, 1020 -> accepted.
inline bool acceptEdge(uint32_t nowUs, bool hasLast, uint32_t lastAcceptedUs, uint32_t glitchUs) {
  return !hasLast || (uint32_t)(nowUs - lastAcceptedUs) >= glitchUs;  // first edge, or far enough apart
}

struct EdgeSnapshot {                 // copy of the interrupt's counters, taken together
  uint32_t count;                     // accepted edges since power up (wraps round)
  uint32_t lastEdgeUs;                // micros() of the newest accepted edge
};

class SpeedEstimator {                // edges -> frequency
public:
  SpeedEstimator(uint32_t stopTimeoutUs = 500000);  // no edge for this long = stopped
  void update(const EdgeSnapshot& snap, uint32_t nowUs);  // call every 100 ms; read nowUs AFTER the snapshot
  uint32_t hzX10() const;             // frequency in tenths of a Hz (0 when stopped)
  bool stopped() const;               // true = disc not turning (starts true)

private:
  uint32_t stopTimeoutUs_;            // stop timeout in us
  bool     primed_;                   // a reference edge has been recorded
  bool     stopped_;                  // not turning
  uint32_t prevCount_;                // edge count at the reference edge
  uint32_t prevEdgeUs_;               // time of the reference edge
  uint32_t hzX10_;                    // current estimate, tenths of a Hz
};

// rpm = Hz * 60 / slots per revolution. Returns 0 if pulsesPerRev is 0, and
// saturates at 32767 so it always fits the int16_t transmit array.
int16_t rpmFromHzX10(uint32_t hzX10, uint16_t pulsesPerRev);

#endif                                // EV_SPEED_H
```

- [ ] **Step 4: Write `esp32-ev/lib/ev_logic/src/speed.cpp`**

```cpp
/*
  Written         15/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  None - pure logic.

  Description:
  Implementation of SpeedEstimator and rpmFromHzX10() (see speed.h for the
  method and a worked example). Values that could overflow 32 bits are
  worked out in 64 bits.

  History:
  V1.0 :- 20261015
  First version.

  Configuration:
  None.
*/
#include "speed.h"                    // declarations

static const uint64_t US_PER_S_X10 = 10000000ULL;  // 1 000 000 us per second, x 10 for tenths of a Hz

static uint32_t saturate32(uint64_t v) {           // 64 bit -> 32 bit without wrapping round
  return v > 0xFFFFFFFFULL ? 0xFFFFFFFFu : (uint32_t)v;  // too big -> biggest 32 bit value
}

SpeedEstimator::SpeedEstimator(uint32_t stopTimeoutUs)
    : stopTimeoutUs_(stopTimeoutUs),  // no edge for this long = stopped
      primed_(false),                 // no reference edge yet
      stopped_(true),                 // starts stopped
      prevCount_(0),                  // reference edge count
      prevEdgeUs_(0),                 // reference edge time
      hzX10_(0) {}                    // 0.0 Hz

void SpeedEstimator::update(const EdgeSnapshot& snap, uint32_t nowUs) {
  uint32_t n = snap.count - prevCount_;  // new edges since the reference (unsigned = wrap safe)

  if (n > 0) {                        // at least one new edge
    if (!primed_) {                   // first edges ever, or the first since a stop
      prevCount_ = snap.count;        // they only set the reference,
      prevEdgeUs_ = snap.lastEdgeUs;  // because their period is unknown
      primed_ = true;                 // the next update can measure
      return;                         // speed stays as it was (0)
    }
    uint32_t spanUs = snap.lastEdgeUs - prevEdgeUs_;  // time those n periods took
    if (spanUs > 0) {                 // never divide by zero
      hzX10_ = saturate32((uint64_t)n * US_PER_S_X10 / spanUs);  // n / span = frequency (x10)
      stopped_ = false;               // the disc is turning
      prevCount_ = snap.count;        // the newest edge is the next reference
      prevEdgeUs_ = snap.lastEdgeUs;  //  "
    }
    return;                           // done
  }

  if (stopped_) return;               // no edges and already stopped: nothing changes

  uint32_t sinceUs = nowUs - prevEdgeUs_;  // time since the last edge (wrap safe)
  if (sinceUs > stopTimeoutUs_) {     // quiet for too long
    stopped_ = true;                  // the disc has stopped
    primed_ = false;                  // the next edge starts a fresh reference
    hzX10_ = 0;                       // report 0
    return;                           // done
  }
  if (sinceUs > 0) {                  // slowing down: at most one edge per sinceUs
    uint32_t capX10 = (uint32_t)(US_PER_S_X10 / sinceUs);  // fastest it can be now
    if (hzX10_ > capX10) hzX10_ = capX10;  // don't over-report while it slows
  }
}

uint32_t SpeedEstimator::hzX10() const { return hzX10_; }  // tenths of a Hz
bool SpeedEstimator::stopped() const { return stopped_; }  // true = not turning

int16_t rpmFromHzX10(uint32_t hzX10, uint16_t pulsesPerRev) {
  if (pulsesPerRev == 0) return 0;    // not set up -> 0 (never divide by zero)
  uint64_t rpm = (uint64_t)hzX10 * 6 / pulsesPerRev;  // Hz x 60 / slots, and hzX10 is Hz x 10
  return rpm > 32767 ? (int16_t)32767 : (int16_t)rpm; // fits the transmit array, never negative
}
```

- [ ] **Step 5: Run the tests and make sure they pass**

Run: `cd esp32-ev && pio test -e native`
Expected: `37 test cases: 37 succeeded`.

- [ ] **Step 6: Commit**

```bash
git add esp32-ev/lib/ev_logic/src/speed.h esp32-ev/lib/ev_logic/src/speed.cpp esp32-ev/test/test_ev_logic/test_main.cpp
git commit -m "Add speed estimator (slotted disc -> Hz -> rpm) with unit tests

Ported from the FS MCU speed estimator: glitch filter, count-over-span
frequency, slowing cap, 0.5 s stop timeout. rpm saturates at 32767.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 7: Explain-it checkpoint (ask the student, wait for answers)**
  1. Why does the first edge after a standstill not give a speed?
  2. In `test_speed_stops_slows_and_restarts`, where does the 48 come from?
  3. Why is `nowUs - prevEdgeUs_` still right when `micros()` wraps to 0?

---

### Task 3: Motor-current conversion in `lib/ev_logic` (O5 logic)

**Files:**
- Modify: `esp32-ev/lib/ev_logic/src/sensors.h`
- Modify: `esp32-ev/lib/ev_logic/src/sensors.cpp`
- Modify: `esp32-ev/test/test_ev_logic/test_main.cpp`

**Interfaces:**
- Produces (used by Task 4): `int16_t motorMilliAmps(uint32_t pinMv, uint16_t mvPerAmp, uint16_t floorMv);`

- [ ] **Step 1: Write the failing test.** Insert this block, followed by one blank line, before `int main() {`:

```cpp
// ---------------------------------------------------------------- Motor current

void test_motor_current_conversion() {
  TEST_ASSERT_EQUAL(0, motorMilliAmps(0, 500, 150));         // 0 V = 0 A
  TEST_ASSERT_EQUAL(0, motorMilliAmps(142, 500, 150));       // ESP32 ADC floor (~142 mV with 0 V in) -> 0 A, not 0.28 A
  TEST_ASSERT_EQUAL(300, motorMilliAmps(150, 500, 150));     // at the floor: measured normally
  TEST_ASSERT_EQUAL(1000, motorMilliAmps(500, 500, 150));    // rig 1 V/A, 10k/10k divider: 0.5 V at the pin = 1 A
  TEST_ASSERT_EQUAL(3300, motorMilliAmps(1650, 500, 150));   // home test: 3V3 into the divider
  TEST_ASSERT_EQUAL(5000, motorMilliAmps(2500, 500, 150));   // home test: 5V into the divider
  TEST_ASSERT_EQUAL(1000, motorMilliAmps(470, 470, 150));    // fallback 0.47 ohm shunt: 470 mV per amp
  TEST_ASSERT_EQUAL(0, motorMilliAmps(1000, 0, 150));        // not set up -> 0, no divide by zero
  TEST_ASSERT_EQUAL(32767, motorMilliAmps(4000000, 1, 150)); // far over range -> saturates, never negative
}
```

Add `RUN_TEST(test_motor_current_conversion);` at the end of the `RUN_TEST` list.

- [ ] **Step 2: Run the tests and make sure they fail**

Run: `cd esp32-ev && pio test -e native`
Expected: build FAILS with `use of undeclared identifier 'motorMilliAmps'` (clang) or `'motorMilliAmps' was not declared in this scope` (gcc).

- [ ] **Step 3: Update `sensors.h`.** Change `Revision:       1.0` to `Revision:       1.1`. Replace the `Description:` paragraph and the `History:` section with:

```
  Description:
  Sensor conversions.
  Temperature (LM35 fitted, TEMP_ENABLED in config.h). Temperatures are kept
  in tenths of a degree (deci-C) so they fit in the integer transmit array:
  23.4 C -> 234.
    LM35  : 10 mV per C, 0 mV at 0 C       -> deciC = mV
    TMP36 : 10 mV per C, 500 mV at 0 C     -> deciC = mV - 500
  Motor current from the DC servo rig. The rig gives 1 V per amp; a 10k/10k
  divider halves it, so the ESP32 pin sees 500 mV per amp:
    mA = pin mV * 1000 / mvPerAmp          (500 mV at the pin -> 1000 mA)
  The ESP32 ADC can't read below about 142 mV (its calibration bottoms out
  there), so anything under floorMv (150 mV) is reported as 0 mA.

  History:
  V1.0 :- 20261008
  First version.
  V1.1 :- 20261015
  motorMilliAmps() for the DC servo rig current sense.
```

and add this declaration after `bool overheat(...)`:

```cpp
int16_t motorMilliAmps(uint32_t pinMv, uint16_t mvPerAmp, uint16_t floorMv);  // pin mV -> mA (0 under floorMv or if mvPerAmp is 0, max 32767)
```

- [ ] **Step 4: Update `sensors.cpp`.** Bump its `Revision` to 1.1, add the history lines `  V1.1 :- 20261015` / `  motorMilliAmps().`, change `Implementation of the temperature functions (see sensors.h).` to `Implementation of the temperature and motor current functions (see sensors.h).`, and add to the end of the file:

```cpp

int16_t motorMilliAmps(uint32_t pinMv, uint16_t mvPerAmp, uint16_t floorMv) {
  if (mvPerAmp == 0) return 0;        // not set up -> 0 (never divide by zero)
  if (pinMv < floorMv) return 0;      // below the ADC's floor = no current (ADC can't read lower)
  uint64_t mA = (uint64_t)pinMv * 1000 / mvPerAmp;  // mV / (mV per A) = A, x 1000 = mA
  return mA > 32767 ? (int16_t)32767 : (int16_t)mA; // fits the transmit array, never negative
}
```

- [ ] **Step 5: Run the tests and make sure they pass**

Run: `cd esp32-ev && pio test -e native`
Expected: `38 test cases: 38 succeeded`.

- [ ] **Step 6: Commit**

```bash
git add esp32-ev/lib/ev_logic/src/sensors.h esp32-ev/lib/ev_logic/src/sensors.cpp esp32-ev/test/test_ev_logic/test_main.cpp
git commit -m "Add motor current conversion (pin mV -> mA) with unit test

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 7: Explain-it checkpoint**
  1. Why is the divisor 500 and not 1000?
  2. Why does 142 mV give 0 mA instead of 284 mA, and what does that mean for small currents?
  3. What would the dashboard show if `IMOTOR_MV_PER_A` were accidentally 0?

---

### Task 4: Carry rpm and current through the car state and the transmit array

**Files:**
- Modify: `esp32-ev/lib/ev_logic/src/vehicle.h`, `vehicle.cpp`
- Modify: `esp32-ev/lib/ev_logic/src/telemetry.h`, `telemetry.cpp`
- Modify: `esp32-ev/web/index.html` (protocol `FIELDS` list + demo line only)
- Modify: `esp32-ev/test/test_ev_logic/test_main.cpp`

**Interfaces:**
- Consumes: `motorMilliAmps()` (Task 3).
- Produces (used by Tasks 5 and 6):
  - `VehicleConfig::motorMvPerAmp` (`uint16_t`, default 500) and `VehicleConfig::motorFloorMv` (`uint16_t`, default 150)
  - `Inputs::rpm` (`int16_t`) and `Inputs::motorMv` (`uint32_t`)
  - `VehicleState::rpm` and `VehicleState::motorMa` (`int16_t`)
  - `TX_RPM` = index 21, `TX_IMA` = index 22, `TX_COUNT` = 23, `TX_LINE_MAX` = 176
  - Header ends `...,s4,js,rpm,imA\n`

- [ ] **Step 1: Write and update the tests.** In `test_telemetry_exact_line`, change the expected string and the token-count comment to:

```cpp
  TEST_ASSERT_EQUAL_STRING("$EV,0,1,2,3,4,5,6,7,-999,9,10,11,12,13,14,15,16,17,18,19,20,21,22\n", buf);
  TEST_ASSERT_EQUAL((int)strlen(buf), n);     // length returned
  TEST_ASSERT_EQUAL(TX_COUNT + 1, countTokens(buf));  // "$EV" + 23 values
```

In `test_telemetry_header_matches_fields` change the comment `// "#EV" + 21 names` to `// "#EV" + 23 names`, and add after the existing asserts:

```cpp
  TEST_ASSERT_NOT_NULL(strstr(h, ",js,rpm,imA\n"));  // new fields at the end, in order
```

Insert this block, followed by one blank line, immediately after the closing `}` of `test_vehicle_tx_array_from_state()` (end of the Whole car section, before the `// ---- Speed sensor` heading):

```cpp
void test_vehicle_rig_values_reach_tx_array() {
  VehicleConfig cfg;                  // default settings (500 mV per amp)
  VehicleState st;                    // the car
  vehicleInit(st, cfg, CENTRE, CENTRE);  // calibrate
  Inputs in; in.ver = CENTRE; in.hor = CENTRE;  // stick centred
  in.rpm = 1500;                      // main.cpp measured 1500 rpm
  in.motorMv = 600;                   // 0.6 V at the current pin
  vehicleUpdate(st, in, 0);           // one step
  TEST_ASSERT_EQUAL(1500, st.rpm);    // speed copied into the state
  TEST_ASSERT_EQUAL(1200, st.motorMa);  // 600 mV / 500 mV per A = 1.2 A
  int16_t tx[TX_COUNT];               // transmit array
  buildTxArray(st, 0, tx);            // fill it
  TEST_ASSERT_EQUAL(1500, tx[TX_RPM]);  // sent to the dashboards
  TEST_ASSERT_EQUAL(1200, tx[TX_IMA]);  //  "
}
```

Add `RUN_TEST(test_vehicle_rig_values_reach_tx_array);` at the end of the `RUN_TEST` list. (`strstr` comes from `<string.h>`, which is already included.)

- [ ] **Step 2: Run the tests and make sure they fail**

Run: `cd esp32-ev && pio test -e native`
Expected: build FAILS: `no member named 'rpm' in 'Inputs'` and `use of undeclared identifier 'TX_RPM'` (clang; gcc words them `'struct Inputs' has no member named 'rpm'` and `'TX_RPM' was not declared`).

- [ ] **Step 3: Update `vehicle.h`.** Bump `Revision` to 1.1, add history `V1.1 :- 20261015` / `DC servo rig: rpm and motor current carried through the state.`, and in the `Description:` paragraph replace the line `  (joystick, LDR, switches) and the time, and works out every output` with the two lines `  (joystick, LDR, switches, rig speed and current) and the time, and` and `  works out every output`. Then:

In `struct VehicleConfig`, after `int16_t  overheatDeciC   = 400;     // 40.0 C overheat warning`, add:

```cpp
  uint16_t motorMvPerAmp   = 500;     // current sense at the ADC pin: rig 1 V/A x 10k/10k divider
  uint16_t motorFloorMv    = 150;     // ESP32 ADC floor: pin readings below this = 0 A
```

In `struct Inputs`, after `bool     js = false;                // joystick button (start/stop)`, add:

```cpp
  int16_t  rpm = 0;                   // rig motor speed (main.cpp measures it every 100 ms)
  uint32_t motorMv = 0;               // rig current sense pin millivolts
```

In `struct VehicleState`, after `uint8_t brake     = 0;              // brake light PWM duty 0..255`, add:

```cpp
  int16_t rpm       = 0;              // rig motor speed in rpm
  int16_t motorMa   = 0;              // rig motor current in mA
```

- [ ] **Step 4: Update `vehicle.cpp`.** Bump `Revision` to 1.1 with the same history line. In the description's order list, after the line `    7 temperature            8 motor + brake light duty` add the line `    9 DC servo rig: speed and current`.

At the end of `vehicleUpdate()`, after the `st.brake = brakeDuty(...)` line, add:

```cpp

  // 9 - DC servo rig: speed arrives already measured, current is converted here
  st.rpm = in.rpm;                    // motor speed from the slotted disc
  st.motorMa = motorMilliAmps(in.motorMv, st.cfg.motorMvPerAmp, st.cfg.motorFloorMv);  // pin mV -> mA
```

- [ ] **Step 5: Update `telemetry.h`.** Bump `Revision` to 1.1 with history `V1.1 :- 20261015` / `rpm and imA appended (fields 21, 22).`. Change the line format in the description to:

```
      $EV,seq,ign,gear,thr,brk,steer,ldr,head,tempdC,hot,indL,indR,lampL,lampR,motor,brake,s1,s2,s3,s4,js,rpm,imA\n
```

Replace the end of the enum:

```cpp
  TX_JS,                              // joystick button pressed 0/1
  TX_RPM,                             // rig motor speed rpm 0..32767
  TX_IMA,                             // rig motor current mA 0..32767
  TX_COUNT                            // number of fields (23)
};

const size_t TX_LINE_MAX = 176;       // longest line: "$EV" + 23 x ",-32768" + "\n" + '\0' = 166
```

- [ ] **Step 6: Update `telemetry.cpp`.** Bump `Revision` to 1.1 with the same history line. In `buildTxArray()`, after the `tx[TX_JS] = ...` line, add:

```cpp
  tx[TX_RPM]    = st.rpm;                          // rig motor speed
  tx[TX_IMA]    = st.motorMa;                      // rig motor current
```

Replace the return line of `txHeader()` with:

```cpp
  return "#EV,seq,ign,gear,thr,brk,steer,ldr,head,tempdC,hot,"
         "indL,indR,lampL,lampR,motor,brake,s1,s2,s3,s4,js,rpm,imA\n";  // field names
```

- [ ] **Step 7: Run the tests and make sure they pass**

Run: `cd esp32-ev && pio test -e native`
Expected: `39 test cases: 39 succeeded`.

- [ ] **Step 8: Keep the web dashboard parsing.** In `esp32-ev/web/index.html`:

In the header comment change `Revision:       1.0` to `Revision:       1.1`, change `(21 integers, 10 Hz)` to `(23 integers, 10 Hz)`, and replace the line `History:        V1.0 :- 20261008  First version.` with the two lines:

```
History:        V1.0 :- 20261008  First version.
                  V1.1 :- 20261015  rpm and imA fields (21, 22) parsed; demo line extended.
```

In `const FIELDS = [`, replace `'js'      // 20 joystick button pressed (ignition start/stop)` with:

```js
  'js',     // 20 joystick button pressed (ignition start/stop)
  'rpm',    // 21 rig motor speed in rpm (slotted disc)
  'imA'     // 22 rig motor current in mA
```

In `parseLine`, change the comment `// must be "$EV" + 21 values = 22 tokens` to `// must be "$EV" + 23 values = 24 tokens`.

In `demoLine()`, replace

```js
      const v = [demo.seq, 1, gear, thr, brk, steer, ldr, demo.head, tempdC, tempdC >= 400 ? 1 : 0,
                 indL, indR, lampL, lampR, motor, brakeDuty, s1, s2, s3, s4, 0];
```

with

```js
      const rpm = Math.round(motor * 3000 / 255);               // rig speed follows the motor duty
      const imA = motor > 0 ? Math.round(300 + motor * 1200 / 255) : 0;  // 0.3..1.5 A while driving
      const v = [demo.seq, 1, gear, thr, brk, steer, ldr, demo.head, tempdC, tempdC >= 400 ? 1 : 0,
                 indL, indR, lampL, lampR, motor, brakeDuty, s1, s2, s3, s4, 0, rpm, imA];
```

Then run `grep -n "21 \|22 tokens" esp32-ev/web/index.html`. Expected: exactly one line, `'rpm',    // 21 rig motor speed in rpm (slotted disc)`.

- [ ] **Step 9: Check the web parser in Node**

Run (from `IES/`):

```bash
node -e '
const h = require("fs").readFileSync("esp32-ev/web/index.html", "utf8");
const js = h.split("<script>")[1].split("</script>")[0];
const parseLine = new Function(js + "; return parseLine;")();
const n = (k) => "$EV," + Array.from({length: k}, (_, i) => i).join(",");
const d = parseLine(n(23));
if (!d || d.rpm !== 21 || d.imA !== 22) { console.error("FAIL 23-field line", d); process.exit(1); }
if (parseLine(n(21)) !== null) { console.error("FAIL: old 21-field line accepted"); process.exit(1); }
console.log("web parseLine OK");'
```

Expected: `web parseLine OK`.

- [ ] **Step 10: Commit**

```bash
git add esp32-ev/lib/ev_logic/src/vehicle.h esp32-ev/lib/ev_logic/src/vehicle.cpp esp32-ev/lib/ev_logic/src/telemetry.h esp32-ev/lib/ev_logic/src/telemetry.cpp esp32-ev/web/index.html esp32-ev/test/test_ev_logic/test_main.cpp
git commit -m "Transmit array: append rpm and imA (23 fields); web parser follows

TX_LINE_MAX 160 -> 176 for the longer worst-case line.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 11: Explain-it checkpoint**
  1. Why are the new fields added at the end instead of next to `motor`?
  2. Work out the 166 in the `TX_LINE_MAX` comment.
  3. What would the web dashboard do with a 23-field line if `FIELDS` hadn't been updated?

---

### Task 5: Firmware glue: pins, settings, speed interrupt, current pin, main loop

**Files:**
- Modify: `esp32-ev/include/pins.h`, `esp32-ev/include/config.h`
- Create: `esp32-ev/src/speed_input.h`, `esp32-ev/src/speed_input.cpp`
- Modify: `esp32-ev/src/main.cpp`

**Interfaces:**
- Consumes: `acceptEdge`, `EdgeSnapshot`, `SpeedEstimator`, `rpmFromHzX10` (Task 2); `Inputs::rpm`, `Inputs::motorMv`, `VehicleConfig::motorMvPerAmp` (Task 4).
- Produces (used by Task 7 docs): `PIN_SPEED 4`, `PIN_IMOTOR 32`, `PIN_SPEED_TEST 14`, and the config names `SPEED_PULSES_PER_REV`, `SPEED_GLITCH_US`, `SPEED_STOP_TIMEOUT_US`, `SPEED_TEST_HZ`, `IMOTOR_MV_PER_A`, `IMOTOR_FLOOR_MV`, `IMOTOR_SAMPLES`; `void speedInputBegin()`, `EdgeSnapshot speedInputSnapshot()`.

- [ ] **Step 1: Add pins.** In `pins.h` bump `Revision` to 1.1, add history `V1.1 :- 20261015` / `DC servo rig pins: speed (4), current (32), test output (14).`, replace the line `     16/17 (PSRAM on the WROVER), 1/3 (USB serial) and 14 (JTAG, pulses at boot).` with the two lines `     16/17 (PSRAM on the WROVER), 1/3 (USB serial) and 14 (JTAG, pulses at` and `     boot - used only as the optional home speed-test output).`, and add this block between the last `#define` and the blank line before `#endif`, with one blank line above it:

```cpp
// ---- DC servo rig (Semester 1 additions) -----------------------------------
#define PIN_SPEED      4              // rig slotted-disc pulses, 0-5 V through a 10k/18k divider -> interrupt
#define PIN_IMOTOR     32             // rig armature current, 1 V/A through a 10k/10k divider -> ADC1_CH4
#define PIN_SPEED_TEST 14             // home testing only: square wave out when SPEED_TEST_HZ > 0 (jumper to GPIO4)
```

- [ ] **Step 2: Add settings.** In `config.h` bump `Revision` to 1.2, add history `V1.2 :- 20261015` / `DC servo rig speed and current settings.`, and add this block between the last `#define` and the blank line before `#endif`, with one blank line above it:

```cpp
// ---- DC servo rig: speed sensor --------------------------------------------
#define SPEED_PULSES_PER_REV  1       // slots per revolution - MEASURE IT in the lab (22 Oct checklist) and set it
#define SPEED_GLITCH_US       20      // edges closer than this are noise (fastest real signal ~25 kHz)
#define SPEED_STOP_TIMEOUT_US 500000  // no edge for 0.5 s = disc stopped, rpm 0
#define SPEED_TEST_HZ         0       // 0 = off. Home test: 200/500/1000 puts a square wave on PIN_SPEED_TEST

// ---- DC servo rig: motor current -------------------------------------------
#define IMOTOR_MV_PER_A       500     // rig 1000 mV/A x 10k/10k divider (0.5) = 500 mV per amp at the pin
#define IMOTOR_FLOOR_MV       150     // ESP32 ADC reads ~142 mV with 0 V in: pin below this = 0 A
#define IMOTOR_SAMPLES        16      // analogReadMilliVolts() readings averaged per loop
```

- [ ] **Step 3: Write `esp32-ev/src/speed_input.h`**

```cpp
/*
  Written         15/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  ESP32-WROVER-E DevKit. Rig speed output (0-5 V square wave) through a
  10k/18k divider into PIN_SPEED (GPIO4).

  Description:
  The only code that touches the speed interrupt. On every rising edge the
  interrupt checks acceptEdge() (lib/ev_logic/src/speed.h), then counts the
  edge and stores its time. main.cpp takes a snapshot every 100 ms and the
  SpeedEstimator turns it into a frequency.
  With SPEED_TEST_HZ > 0 it also puts a square wave on PIN_SPEED_TEST, so
  the whole chain can be tested at home with a jumper from GPIO14 to GPIO4.

  History:
  V1.0 :- 20261015
  First version.

  Configuration:
  SPEED_GLITCH_US and SPEED_TEST_HZ in include/config.h.
*/
#ifndef EV_SPEED_INPUT_H              // include guard
#define EV_SPEED_INPUT_H              // mark as included

#include "speed.h"                    // EdgeSnapshot (lib/ev_logic)

void speedInputBegin();               // attach the interrupt (and the test wave) - call in setup()
EdgeSnapshot speedInputSnapshot();    // copy the edge count and last edge time together, safely

#endif                                // EV_SPEED_INPUT_H
```

- [ ] **Step 4: Write `esp32-ev/src/speed_input.cpp`**

```cpp
/*
  Written         15/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  ESP32-WROVER-E DevKit, PIN_SPEED (GPIO4) and PIN_SPEED_TEST (GPIO14).

  Description:
  Speed interrupt and snapshot (see speed_input.h).
  ISR = interrupt service routine: a short function the chip runs the
  instant an edge arrives, pausing loop() for a few microseconds. It only
  counts and timestamps. The maths happens later in the loop.
  The counters are shared between the ISR and loop(), so both sides use a
  spinlock (portMUX). The snapshot then always holds a count and a time
  from the same moment.
  IRAM_ATTR puts the ISR in the chip's fast internal RAM so it starts
  straight away.

  History:
  V1.0 :- 20261015
  First version.

  Configuration:
  None.
*/
#include <Arduino.h>                  // attachInterrupt, micros, ledcAttach, ledcWrite
#include "pins.h"                     // PIN_SPEED, PIN_SPEED_TEST
#include "config.h"                   // SPEED_GLITCH_US, SPEED_TEST_HZ
#include "speed_input.h"              // our declarations

static portMUX_TYPE speedMux = portMUX_INITIALIZER_UNLOCKED;  // lock shared by the ISR and loop()
static volatile uint32_t edgeCount = 0;   // accepted edges since power up (wraps round)
static volatile uint32_t lastEdgeUs = 0;  // micros() of the last accepted edge
static volatile bool haveEdge = false;    // true once lastEdgeUs holds a real edge

static void IRAM_ATTR speedIsr() {    // runs on every rising edge on PIN_SPEED
  uint32_t now = micros();            // time of this edge in us
  portENTER_CRITICAL_ISR(&speedMux);  // take the lock (ISR version)
  if (acceptEdge(now, haveEdge, lastEdgeUs, SPEED_GLITCH_US)) {  // not noise?
    edgeCount = edgeCount + 1;        // count it (not ++: C++20 deprecates ++ on volatile)
    lastEdgeUs = now;                 // remember when
    haveEdge = true;                  // there is a last edge now
  }
  portEXIT_CRITICAL_ISR(&speedMux);   // release the lock
}

void speedInputBegin() {
  pinMode(PIN_SPEED, INPUT);          // plain input: the divider's 18k holds it LOW with nothing connected
  attachInterrupt(digitalPinToInterrupt(PIN_SPEED), speedIsr, RISING);  // call speedIsr on low-to-high
  if (SPEED_TEST_HZ > 0) {            // home test wave wanted?
    ledcAttach(PIN_SPEED_TEST, SPEED_TEST_HZ, 8);  // PWM at SPEED_TEST_HZ, 8 bit duty
    ledcWrite(PIN_SPEED_TEST, 128);   // 128/256 = 50 % duty square wave
  }
}

EdgeSnapshot speedInputSnapshot() {
  EdgeSnapshot s;                     // the copy
  portENTER_CRITICAL(&speedMux);      // block the ISR for a moment
  s.count = edgeCount;                // both values from the same instant
  s.lastEdgeUs = lastEdgeUs;          //  "
  portEXIT_CRITICAL(&speedMux);       // let the ISR run again
  return s;                           // hand it back
}
```

- [ ] **Step 5: Wire it into `main.cpp`.** Bump `Revision` to 2.1. After the history lines ending `WiFi web dashboard added.` add `  V2.1 :- 20261015` / `  DC servo rig: speed interrupt -> rpm every 100 ms, motor current pin,` / `  both in the transmit array.`. After the line `   - Push switches S1 (gear up), S3 (gear down), S4 (left), S2 (right)` add the two lines `   - DC servo rig: speed pulses on GPIO4, armature current on GPIO32` and `     (see docs/wiring.md)`. Then:

After `#include "web_dashboard.h"            // WiFi web dashboard` add:

```cpp
#include "speed.h"                    // SpeedEstimator, rpmFromHzX10 (lib/ev_logic)
#include "speed_input.h"              // speed interrupt
```

After `static uint16_t txSeq = 0;            // transmit counter` add:

```cpp
static SpeedEstimator speedEst(SPEED_STOP_TIMEOUT_US);  // disc edges -> frequency
static int16_t rpmNow = 0;            // latest motor speed, updated every 100 ms
```

After the `readAveraged()` function add:

```cpp
// Read one analog pin in calibrated millivolts several times and average.
static uint32_t readMilliVoltsAveraged(uint8_t pin, int samples) {
  uint32_t sum = 0;                   // running total
  for (int i = 0; i < samples; i++) { // take several readings
    sum += analogReadMilliVolts(pin); // calibrated mV (uses the chip's eFuse calibration)
  }
  return sum / samples;               // average
}

// Turn the interrupt's edge count into rpm. Called once per 100 ms.
static void updateSpeed() {
  EdgeSnapshot snap = speedInputSnapshot();  // copy the counters first...
  speedEst.update(snap, micros());    // ...then read the time, so it is never before the last edge
  rpmNow = rpmFromHzX10(speedEst.hzX10(), SPEED_PULSES_PER_REV);  // Hz -> rpm
}
```

In `makeConfig()`, after `cfg.overheatDeciC   = OVERHEAT_DECI_C;   // overheat limit` add:

```cpp
  cfg.motorMvPerAmp   = IMOTOR_MV_PER_A;   // current sense scaling
  cfg.motorFloorMv    = IMOTOR_FLOOR_MV;   // ADC floor
```

In `readInputs()`, after `in.js = digitalRead(PIN_JS_SEL) == LOW;  //  "` add:

```cpp
  in.rpm = rpmNow;                         // measured in updateSpeed()
  in.motorMv = readMilliVoltsAveraged(PIN_IMOTOR, IMOTOR_SAMPLES);  // current sense pin
```

In `setup()`, after `analogWrite(PIN_BRAKE, 0);          // attach PWM with the brake light off` add:

```cpp
  speedInputBegin();                  // speed interrupt (and the test wave if SPEED_TEST_HZ > 0)
```

Replace the whole `loop()` with:

```cpp
void loop() {
  uint32_t now = millis();            // time now (ms since power up)
  bool txDue = now - lastTx >= TX_PERIOD_MS;  // time to measure speed and send?
  if (txDue) {                        // once every 100 ms
    lastTx = now;                     // remember when
    updateSpeed();                    // 0 - new rpm from the disc
  }
  Inputs in = readInputs();           // 1 - read every input
  vehicleUpdate(car, in, now);        // 2 - work out what the car should do
  driveOutputs();                     // 3 - set motor, lights and indicators
  if (txDue) transmit();              // 4 - send the transmit array
  handleSerialCommand();              // 5 - check for '?' from the PC
  if (WEB_ENABLED) webLoop();         // 6 - web housekeeping
}
```

Also, in the main description, replace the line `  Every 100 ms: build the TRANSMIT ARRAY and send it as one line` with these two lines (the lines after it stay as they are):

```
  Every 100 ms: work out the rig motor rpm from the speed interrupt's count,
                and build the TRANSMIT ARRAY and send it as one line
```

- [ ] **Step 6: Build the firmware and run the tests**

Run: `cd esp32-ev && pio run -e esp32 && pio test -e native`
Expected: `[SUCCESS]` for esp32 (RAM/Flash usage printed) and `39 test cases: 39 succeeded`.

- [ ] **Step 7: Commit (do not push yet: the Processing sketch still expects 21 fields until Task 6)**

```bash
git add esp32-ev/include/pins.h esp32-ev/include/config.h esp32-ev/src/speed_input.h esp32-ev/src/speed_input.cpp esp32-ev/src/main.cpp
git commit -m "Firmware: rig speed interrupt on GPIO4, current sense on GPIO32

rpm worked out every 100 ms from the interrupt's edge count. Optional
test square wave on GPIO14 (SPEED_TEST_HZ) for testing at home.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 8: Explain-it checkpoint**
  1. Why does the ISR only count and timestamp, and not work out the rpm?
  2. What could go wrong without the `portENTER_CRITICAL` in `speedInputSnapshot()`?
  3. Why is `micros()` read after the snapshot, not before?

---

### Task 6: Processing dashboard: rig panel, 23 fields, bad-line counter (O6)

**Files:**
- Modify: `dashboard/EV_Dashboard/EV_Dashboard.pde`

**Interfaces:**
- Consumes: field indexes 21 (`rpm`) and 22 (`imA`), 23 fields per line (Task 4).

- [ ] **Step 1: Header comment.** Bump `Revision` to 1.1, add history `V1.1 :- 20261015` / `DC servo rig panel (rpm, motor current), 23 fields, bad-line counter.`. Change `in the form  $EV,<21 comma-separated integers>` to `in the form  $EV,<23 comma-separated integers>`, and add to the bullet list after `- a 7-segment digit that copies the physical lamps`:

```
    - DC servo rig panel: motor speed (rpm) and motor current (A)
```

- [ ] **Step 2: Constants.** Change `final int NUM_FIELDS      = 21;` to `final int NUM_FIELDS      = 23;`. After `final int F_JS    = 20;                        // joystick button pressed (start / stop)` add:

```java
final int F_RPM   = 21;                        // rig motor speed in rpm
final int F_IMA   = 22;                        // rig motor current in mA

final int RPM_FULL_SCALE     = 3000;           // full bar; set to the rig's rated rpm once the manual confirms it
final int AMPS_FULL_SCALE_MA = 3000;           // 3 A = full current bar
int badLines = 0;                              // $EV lines rejected (wrong field count or not numbers)
```

- [ ] **Step 3: Window and draw order.** In `settings()` change `size(1000, 620);` to `size(1000, 760);`. In `draw()`, after `drawTemperature(d);                          // temperature read-out` add:

```java
  drawRig(d);                                  // DC servo rig speed and current
```

- [ ] **Step 4: Panel.** In `drawPanels()`, after `rect(580, 455, 400, 120, 10);                // bottom right: temperature` add:

```java
  rect(20, 590, 960, 120, 10);                 // bottom row: DC servo rig
```

and after `text("TEMPERATURE", 595, 472);               // temperature panel title` add:

```java
  text("DC SERVO RIG", 35, 607);               // rig panel title
```

- [ ] **Step 5: Draw functions.** Add after the end of `drawTemperature()`:

```java
void drawRig(int[] d) {                        // DC servo rig: motor speed and current
  int rpm = max(d[F_RPM], 0);                  // motor speed in rpm, never below 0
  int mA = max(d[F_IMA], 0);                   // motor current in mA, never below 0
  fill(cDim);                                  // label colour
  textAlign(LEFT, CENTER);                     // labels start at their x position
  textSize(11);                                // small label text
  text("MOTOR SPEED (slotted disc)", 45, 625); // left label
  text("MOTOR CURRENT (1 V/A output)", 535, 625);  // right label
  drawHBar(45, 640, 420, 24, rpm * 100 / RPM_FULL_SCALE, cGreen);       // speed bar
  drawHBar(535, 640, 420, 24, mA * 100 / AMPS_FULL_SCALE_MA, cAmber);   // current bar
  fill(cText);                                 // value colour
  textSize(28);                                // big numbers
  text(rpm + " rpm", 45, 688);                 // e.g. 1500 rpm
  text(nf(mA / 1000.0, 0, 2) + " A", 535, 688);  // e.g. 1.20 A
}                                              // end of drawRig()


void drawHBar(float x, float y, float w, float h, int pct, color c) {  // one horizontal % bar
  pct = constrain(pct, 0, 100);                // keep the value inside 0..100
  noStroke();                                  // no outlines
  fill(cOff);                                  // empty track colour
  rect(x, y, w, h, 4);                         // empty track
  fill(c);                                     // bar colour
  rect(x, y, w * pct / 100.0, h, 4);           // filled part grows to the right
}                                              // end of drawHBar()
```

- [ ] **Step 6: Status bar at the new bottom, with the bad-line counter.** In `drawStatusBar()`, replace these five lines (keeping their comments at column 48 where they fit):

```java
  rect(0, height - 32, width, 32);             // full-width bar along the bottom
  text(s, 15, height - 16);                    // draw the status text
    text("  STALE", 15 + textWidth(s), height - 16);  // warning straight after the port name
  text("SEQ " + d[F_SEQ], 500, height - 16);   // sequence number of the last line
  text("P: next port   D: demo   H: header", 985, height - 16);  // key help
```

(they replace the lines that use `588` and `604`; the STALE line keeps its existing indent inside the `if`). After the closing `}` of the `if (isStale())` block add:

```java
  if (badLines > 0) {                          // lines with the wrong field count arrived?
    fill(cRed);                                // red
    textAlign(LEFT, CENTER);                   // left-aligned
    text("BAD LINES " + badLines, 560, height - 16);  // usually: firmware and dashboard field counts differ
  }                                            // end of bad line check
```

- [ ] **Step 7: Count rejected lines.** In `serialEvent()`, in the length-check block, change

```java
    if (tok.length != NUM_FIELDS + 1 || !tok[0].equals("$EV")) {  // wrong number of fields?
      return;                                  // ignore half lines
```

to

```java
    if (tok.length != NUM_FIELDS + 1 || !tok[0].equals("$EV")) {  // wrong number of fields?
      badLines++;                              // count it so the status bar can warn
      return;                                  // ignore half lines
```

and change the catch block body comment `// ignore this line and wait for the next one` to:

```java
    badLines++;                                // a field was not a whole number: count it, wait for the next line
```

- [ ] **Step 8: Demo data.** In `updateDemo()`, before `data = d;                                    // swap in the whole array, like serialEvent()` add:

```java
  d[F_RPM] = d[F_MOTOR] * RPM_FULL_SCALE / 255;  // rig speed follows the motor duty
  d[F_IMA] = (d[F_MOTOR] > 0) ? 300 + d[F_MOTOR] * 1200 / 255 : 0;  // 0.3..1.5 A while driving
```

- [ ] **Step 9: Build the sketch from the command line**

Run (from `IES/`; the output folder is a scratch folder outside the repo):

```bash
/Applications/Processing.app/Contents/MacOS/Processing cli --sketch="$PWD/dashboard/EV_Dashboard" --output="$TMPDIR/ev_dashboard_build" --force --build
```

Expected: `Finished.` with no errors (the `sun.awt.X11` warning is normal on macOS).

- [ ] **Step 10: Visual check (controller or student).** Run the sketch (`--run` instead of `--build`, or open it in Processing). With no board connected it starts in demo mode. Check that the DC SERVO RIG panel shows rpm and amps that move with the motor gauge, the status bar sits at the bottom, and nothing overlaps.

- [ ] **Step 11: Commit and push (firmware from Task 5 and this sketch reach `main` together)**

```bash
git add dashboard/EV_Dashboard/EV_Dashboard.pde
git commit -m "Processing dashboard: DC servo rig panel, 23 fields, bad-line counter

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

- [ ] **Step 12: Explain-it checkpoint**
  1. What does the dashboard do with a 21-field line from old firmware now, and what do you see?
  2. Why does the demo code go through the same `data = d` swap as real data?
  3. Where would you change the full-scale value if the rig turns faster than 3000 rpm?

---

### Task 7: Hardware docs: wiring and calculations, home tests, lab checklists, README

**Files:**
- Create: `docs/wiring.md`
- Create: `docs/home-tests.md`
- Create: `docs/lab/2026-10-22-speed-sensor.md`
- Create: `docs/lab/2026-11-05-rig-drive-current.md`
- Modify: `esp32-ev/README.md` (pin map, pins-avoided note, transmit array, new section)

**Interfaces:**
- Consumes: pin and config names from Task 5; field names from Task 4.

- [ ] **Step 1: Write `docs/wiring.md`**

````markdown
# DC servo rig wiring and calculations

Rig: Feedback Mechanical Unit 33-100. Speed output: 0–5 V square wave. Motor: 24 V DC
(rated speed and gearbox ratio: confirm from the manual). Armature current output: 1 V per
amp (maximum and polarity: measured on 22 Oct). ESP32 pins are 3.3 V: **never put more
than 3.6 V on any pin.**

## 1. Speed input: rig square wave → GPIO4

```
rig SPEED OUT (0–5 V) ──[ R1 10 kΩ 1 % ]──●── GPIO4 (PIN_SPEED)
                                          │
                                    [ R2 18 kΩ 1 % ]
                                          │
rig 0 V ──────────────────────────────────●── ESP32 GND
```

- High level at the pin: 5 V × 18 / (10 + 18) = **3.21 V**. That's above the ESP32's
  logic-high threshold (0.75 × 3.3 = 2.48 V) and below its 3.6 V limit. Low has to be
  under 0.25 × 3.3 = 0.83 V.
- Use 1 % resistors. Worst case at 5.25 V in: 5.25 × 18.18 / (9.9 + 18.18) = 3.40 V,
  still under 3.6 V. (No 18 kΩ? Two 10 kΩ in series for R2 give 3.33 V at 5 V in, but
  only if the rig's high level is 5.0 V or less.)
- With nothing connected, R2 holds GPIO4 at 0 V, so there are no false edges.
- The divider loads the rig output with 28 kΩ. If the output is only a weak pull-up, the
  loaded high level can drop below 2.48 V. The 22 Oct checklist (step 6) measures it with
  the divider connected, before it goes to GPIO4.
- Speed: R1∥R2 = 6.4 kΩ, and with ~10 pF of pin capacitance that's 64 ns. Far faster than
  any edge from the disc.
- The grounds must be joined. Keep the speed wire short and twisted with its ground wire,
  away from the motor wires.

## 2. Current input: rig 1 V/A output → GPIO32

```
rig CURRENT OUT (1 V/A) ──[ R3 10 kΩ ]──●────────────●─────────────●── GPIO32 (PIN_IMOTOR)
                                        │            │             │
                                   [ R4 10 kΩ ]  [ C1 1 µF ]   D1 BAT85: anode on this node, cathode on 3V3
                                        │            │         D3 BAT85: anode on GND, cathode on this node
rig 0 V ────────────────────────────────●────────────●── GND
```

- The divider halves the voltage: 1 A → 1 V → **0.5 V at the pin**, so `IMOTOR_MV_PER_A`
  is 500. The ESP32 ADC (12 dB attenuation, the Arduino default) is only accurate from about
  0.15 V to 2.45 V at the pin, so this divider measures about **0.3 A to 4.9 A**.
- Below 0.15 V, `analogReadMilliVolts()` does not go down to 0: the chip's calibration
  bottoms out at about 142 mV (which would read as 0.28 A). The firmware therefore reports
  anything under `IMOTOR_FLOOR_MV` (150 mV) as 0 A.
- If 22 Oct shows the rig output never goes above 2.4 V: leave out R4 and set
  `IMOTOR_MV_PER_A 1000`. That measures 0.15–2.45 A, with better resolution for small
  currents.
- (The spec suggested sizing for about 3.0 V at 5 V. 2.5 V was chosen instead, because the
  ESP32 ADC is most accurate below 2.45 V.)
- C1 with R3∥R4 = 5 kΩ gives a 5 ms time constant (a 32 Hz filter). It averages the 1 kHz
  PWM ripple in the motor current.
- D1 clamps the pin to about 3.6 V if the output is higher than expected. D3 stops it going
  below about −0.3 V if the output turns out to be negative. Use BAT85s, **not a 1N5819**:
  on this 5 kΩ node, every 1 µA of diode leakage adds 5 mV, which reads as 10 mA of false
  current. A 1 A Schottky can leak up to 1 mA; a BAT85 leaks under 2 µA. Keep D1 and D3
  until the real maximum is measured.
- **Check before connecting:** measure the rig output with a meter at full speed. It must
  be positive and below 5 V.

**Fallback (if the rig's 1 V/A output doesn't follow our TIP121 drive):** put a
0.47 Ω 2 W resistor between the TIP121 emitter and the ground star point (section 3).
Feed the emitter to GPIO32 through 10 kΩ, keeping C1, D1 and D3 and leaving out R4, and set
`IMOTOR_MV_PER_A 470`. 1 A then gives 0.47 V and 0.47 W in the resistor. Use the next
smaller base resistor from the table in section 3, because the shunt's voltage drop takes
away base drive.

## 3. Motor drive: TIP121 low-side switch

```
+24 V ──────●──────────────●──────────────┐
            │              │              │
    C2 470 µF 35 V     D2 1N5822        MOTOR (rig, 24 V)
            │      (stripe to +24 V)      │
            │              └──────●───────┘
            │                     │
            │                 C (TIP121, on a heatsink)
            │   GPIO23 ──[ RB ]── B
            │                 E (TIP121) = STAR POINT
            │                     │
24 V minus ─┴──── thick wire ─────┘
ESP32 GND ───── own thin wire ──── STAR POINT
rig 0 V ─────── own thin wire ──── STAR POINT
```

- The TIP121 is a Darlington (two transistors in one) with a current gain (hFE) of at
  least 1000. It switches only the motor's negative side, so the ESP32 never sees 24 V.
- **Base resistor RB:** a Darlington needs a forced gain of about 250 to saturate (the
  datasheet gives VCE(sat) at 3 A with 12 mA base current = 250).
  RB = (3.2 V GPIO high − 1.5 V VBE) / (Imax / 250).

  | Motor full-speed current (measured 22 Oct) | Base current | RB |
  |---|---|---|
  | up to 1.2 A | 3.6 mA | 470 Ω |
  | up to 1.6 A | 5 mA | 330 Ω |
  | up to 2 A | 7.7 mA | 220 Ω |

  All of these are far below the GPIO's 40 mA limit. Start with 470 Ω. With the shunt
  fallback, go one row down.
- **Flyback diode D2 (1N5822, a 3 A 40 V Schottky; UF4004 also OK):** when the transistor
  switches off, the motor's inductance keeps current flowing. The diode gives it a path
  instead of a voltage spike that would kill the TIP121. At low duty the diode carries
  nearly all the motor current, so it needs a 3 A rating. Use a fast diode, not a 1N4007:
  at 1 kHz a slow diode briefly conducts backwards at every turn-on.
- **C2 (470 µF 35 V), right next to the TIP121:** supplies the fast PWM current pulses so
  they don't have to travel down the supply leads.
- **Ground wiring: one star point at the TIP121 emitter.** Run a thick wire (≥ 0.5 mm², not
  a breadboard jumper) from the emitter straight to the 24 V supply minus. The motor current
  then flows only in that wire, the motor leads, D2 and C2. The ESP32 GND and the rig 0 V
  join the star point by their own thin wires. Motor current must never flow through the
  breadboard GND rail, the ESP32 board or the USB cable: 0.1 Ω of jumper × 1 A = 0.1 V,
  which is 0.2 A of error on the current reading and noise on the speed input.
- **Heat:** the TIP121 drops about 1 V when on. At 1 A that's about 1 W, and a bare TO-220
  heats by about 62 °C per watt, so **fit a clip-on heatsink if the motor draws more than
  0.4 A**.
- **24 V supply:** a bench supply set to 24 V with a **2 A current limit** (or the rig's own
  motor supply, only if the technician confirms it).
- **Never connect two drivers to one motor.** The rig normally drives its motor itself.
  Disconnect the rig's drive (ask the technician how) before connecting the TIP121.

**Fallback (no external drive allowed, or the TIP121 stage fails):** drive the rig's own
amplifier input with a steady voltage made from the PWM. GPIO23 → 10 kΩ → node → rig
amplifier input, with 10 µF from the node to GND. That's a 0.1 s time constant, so the node
sits at a DC level of 0–3.3 V that follows the duty. Get the input's voltage range from the
technician or manual first, and check with a meter that the input doesn't push more than
3.3 V back towards the ESP32. The firmware doesn't change.

## 4. Power-up and power-down order

1. Everything off. Join all grounds first at the star point (section 3): 24 V minus by the
   thick wire, ESP32 GND and rig 0 V by their own wires.
2. ESP32 on (USB). Joystick centred. Ignition off.
3. 24 V on.
4. Power down: 24 V off first, then **unplug the rig speed and current leads**, then USB.
   If the ESP32 is unpowered while a rig output is live, that output feeds the 3V3 rail
   through D1 and GPIO4's protection diode.

## 5. New pins

| Function | GPIO | Notes |
|---|---|---|
| Speed pulses | 4 | interrupt, through the 10k/18k divider |
| Motor current | 32 | ADC1_CH4, through the 10k/10k divider, 1 µF, BAT85 clamps |
| Motor PWM | 23 | unchanged pin; drives the TIP121 through RB (section 3 table) |
| Speed test output | 14 | home testing only (`SPEED_TEST_HZ`), jumper to the GPIO4 node |
````

- [ ] **Step 2: Write `docs/home-tests.md`**

```markdown
# Home tests (no rig needed): prove each chain before lab day

Write every result in the logbook the same day.

## A. Speed chain (before 22 Oct)

1. Build the 10k/18k divider (1 %) from `docs/wiring.md` section 1 first. R2 must be in
   place, or GPIO4 floats when the jumper comes out in step 6.
2. In `config.h` set `SPEED_TEST_HZ 200` and `SPEED_PULSES_PER_REV 60`. With 60 "slots",
   rpm reads the same number as the frequency in Hz.
3. Fit a jumper from **GPIO14 to the divider midpoint (the GPIO4 side)**, not the rig side.
4. Flash, open the Processing dashboard. Expected rpm: **200 ±2**.
5. Repeat with `SPEED_TEST_HZ` 500 (expect 500 ±5) and 1000 (expect 1000 ±10).
6. Pull the jumper out. rpm must drop to **0 within 1 s** and stay 0.
7. Divider check: unplug the midpoint from GPIO4. Meter the DevKit's 5V pin (call it V5;
   usually 4.75–5.25 V on USB). Connect 5V to R1's rig side and meter the midpoint. Expect
   **V5 × 0.643 ±2 %** (3.21 V at 5.00 V). Reconnect the midpoint to GPIO4.
8. Set `SPEED_TEST_HZ 0` again before lab day. `SPEED_PULSES_PER_REV` gets set in the lab.

With core 3.3.12, `ledcAttach()` shares a PWM timer only with a pin already running at the
same frequency and resolution, and otherwise takes a free timer. The 200/500 Hz test wave
gets its own timer, and the motor and brake stay at 1 kHz (at 1000 Hz all three share one
timer, which is fine). If rpm doesn't follow `SPEED_TEST_HZ`, check that you re-flashed and
that the serial log shows no `ledc` errors.

## B. Current chain (before 22 Oct)

Build R3, R4, C1, D1 and D3 from `docs/wiring.md` section 2. Feed the input (R3's free end):

| Input | Pin should read (meter) | Dashboard should show |
|---|---|---|
| GND | 0 V | 0.00 A (anything under 0.30 A shows 0: the ADC floor) |
| 3V3 | 1.65 V | 3.30 A (2.97–3.63) |
| 5V | V5 ÷ 2 (about 2.5 V) | V5 ÷ 1 V/A, e.g. 5.00 A (±10 %) |

## C. TIP121 stage on the kit motor (reading week)

Build the stage from `docs/wiring.md` section 3 on the breadboard (470 Ω base resistor,
1N5822, 470 µF, star ground), but run the **kit motor from 5 V**, not 24 V.

1. Ignition on, gear 1, throttle up: the motor starts. Gear 4, full throttle: full speed.
2. Brake: the motor stops at once.
3. Full throttle: measure the collector–emitter voltage (expect under about 1.2 V), and
   touch the TIP121 after a minute (warm is fine; too hot to touch is not).
4. If the ESP32 resets (`Brownout detector was triggered` in the monitor) or the USB port
   drops out when the motor starts, power the kit motor from a separate 5 V supply (or a USB
   power bank). Join its minus to the emitter star point, and keep the ESP32 GND wired to
   that same point.
5. Noise pre-check: set `SPEED_TEST_HZ 500`, jumper GPIO14 → divider midpoint, and run the
   speed wire right next to the motor leads. Drive the kit motor at gear 4, full throttle:
   rpm must stay **500 ±5**. If it jumps, raise `SPEED_GLITCH_US` to 50 and log it. Set
   `SPEED_TEST_HZ 0` afterwards.
```

- [ ] **Step 3: Write `docs/lab/2026-10-22-speed-sensor.md`**

```markdown
# Lab, Thu 22 Oct: speed sensor on the rig (spec O3)

Done when: dashboard rpm agrees within **±5 % at 3 speeds** with (a) the scope frequency
× 60 ÷ slots and (b) the rig's own display converted to the disc's shaft; and the pulse
level and slots per revolution are measured and logged.

## Bring

Laptop (Processing, PlatformIO, repo up to date) · ESP32 + USB cable · breadboard with the
10k/18k speed divider and the current divider (R3 R4 C1, BAT85 D1 and D3), **both already
passed home tests A and B** · jumper wires · 4 mm banana-to-jumper leads · multimeter ·
phone for photos · A3 poster (if it's due today).

## Steps

| # | Do | Record |
|---|---|---|
| 1 | Collect anything still unanswered from H0: manual, speed-output socket, slots and which shaft the disc is on, gearbox ratio, display units (rpm or volts), current-output range, motor terminals, external drive on 5 Nov | Answers |
| 2 | Rig on, **ESP32 not connected**. Scope the speed output at three rig speeds set with the rig's own speed knob | V high, V low, frequency at each speed |
| 3 | Confirm V high is 4.5–5.25 V and V low ≤ 1.0 V. **If V high is above 5.25 V, stop**: with resistor tolerance the pin could pass the ESP32's 3.6 V limit, so recalculate the divider (worst case ≤ 3.4 V at the pin) | |
| 4 | Slots per rev: **count the slots on the disc** (a whole number, it goes in `config.h`). Note which shaft the disc is on, which shaft the display measures, and the gearbox ratio (don't assume one). If the display shows volts, get the V per 1000 rpm. Cross-check: slots ≈ scope frequency × 60 ÷ rpm of the disc's shaft | slots/rev, ratio, display units |
| 5 | Set `SPEED_PULSES_PER_REV` in `config.h`, flash | |
| 6 | Join grounds first (rig 0 V → ESP32 GND). Connect speed output → R1 with the midpoint **not yet wired to GPIO4**. Run the disc at its slowest and meter the midpoint on DC. High must be 2.6–3.4 V and low ≤ 0.6 V. If high is under 2.6 V, the divider is loading a weak pull-up: stop and recalculate with Claude using the unloaded (step 2) and loaded levels. Only then wire the midpoint to GPIO4 | V high / low at the midpoint, loaded |
| 7 | Three speeds (low, middle, high): dashboard rpm vs (a) scope frequency × 60 ÷ slots, and (b) the rig display converted to the disc's shaft using the ratio from step 4. Pass = both within ±5 % | table: scope, rig, ours, error % |
| 8 | Meter the current output: motor stopped, slow, fast, and with the rig's load if it has one. Also note the full-speed current under the rig's own drive (it picks RB, wiring.md §3) | V at each, polarity, full-speed A |
| 9 | If step 8 shows a positive voltage below 5 V: connect it through the current divider and compare amps with the rig's armature-current reading (under 0.3 A shows 0) | table |
| 10 | Photos of the setup, disc and wiring (logbook, report, poster) | |

## If something's wrong

| Symptom | Check |
|---|---|
| rpm stays 0 | Grounds joined? Right socket? With the disc turning, a DC meter on GPIO4 reads 3.2 V × the duty cycle: anywhere from about 0.3 V to 2.9 V (about 1.6 V only if slots and bars are the same width). A steady 0 V or 3.2 V means no pulses arrive; recheck step 6 |
| rpm about double | Counting noise or both edges: raise `SPEED_GLITCH_US` to 50, check the twisted pair |
| rpm off by a fixed factor | `SPEED_PULSES_PER_REV` wrong, or the display measures the other shaft (use the step 4 ratio) |
| rpm jumps around | Loose wire; slots spaced unevenly (average over 1 s by eye and note it) |
```

- [ ] **Step 4: Write `docs/lab/2026-11-05-rig-drive-current.md`**

```markdown
# Lab, Thu 5 Nov: TIP121 drive and current sense on the rig (spec O4, O5)

Done when: the joystick throttle drives the rig motor (0 % = stopped, brake = stopped), and
dashboard amps agree with a meter within **±10 % at 2 loads**.

## Bring

Everything from 22 Oct, plus: TIP121 stage built and tested at home (home test C), with RB
chosen from the 22 Oct full-speed current (wiring.md §3 table), 1N5822, 470 µF 35 V,
heatsink and the thick star-ground wire · bench-supply leads · second multimeter set to its
**10 A socket and range** (the mA socket's fuse blows above a few hundred mA) · shunt
fallback parts (0.47 Ω 2 W resistor, 10 kΩ, the next smaller RB) · amplifier-input fallback
parts (10 kΩ, 10 µF, leads).

## Steps

| # | Do | Record |
|---|---|---|
| 1 | Technician: how to disconnect the rig's own drive; which 24 V supply to use; does the 1 V/A output still work with an external drive? **If the rig's drive can't be disconnected, do NOT connect the TIP121**: use the amplifier-input fallback (wiring.md §3). Get its input range first and check the input doesn't push more than 3.3 V back | Answers, which drive was used |
| 2 | Everything off. Wire per wiring.md §3, including the star ground. Bench supply 24 V, **current limit 2 A** | Photo |
| 3 | Power up in order (wiring.md §4). Ignition on, gear 1, throttle from 0 | Motor starts? Duty at start |
| 4 | Gear 4: throttle at 25, 50, 75, 100 % | motor duty, rpm, amps (ours), amps (meter) |
| 5 | Brake at full speed | Motor stops, brake light on |
| 6 | Speed sensor connected, motor driven by the TIP121: is rpm steady? | Steady / jumpy (if jumpy, see the 22 Oct table) |
| 7 | Two loads drawing more than 0.4 A (rig load control, or two speeds if there's none): dashboard amps vs meter | table, error % |
| 8 | Touch-check the TIP121 after 2 min at full speed | warm / hot |
| 9 | If dashboard amps don't follow the meter at all, the rig's 1 V/A output only measures its own drive. **Do it now:** power down, fit the 0.47 Ω shunt (wiring.md §2 fallback, next smaller RB), set `IMOTOR_MV_PER_A 470`, reflash, repeat step 7 | |
| 10 | Power down in order: 24 V off, unplug the rig leads, then USB. Photos | |
```

- [ ] **Step 5: Update `esp32-ev/README.md`.** In the pin map table, change the motor row to `| Motor PWM (1k → TIP120 base; 470 Ω → TIP121 on the rig) | P1_6 | 23 | 23 | PWM |` and add after the brake light row:

```markdown
| Rig speed pulses (10k/18k divider) | — | 4 | 4 | input, interrupt |
| Rig motor current (10k/10k divider) | — | 32 | 32 | analog (mV) |
| Speed test output (home tests only) | — | 14 | 14 | PWM when `SPEED_TEST_HZ` > 0 |
```

In "Pins avoided", change `  - 14: pulses at boot` to `  - 14: pulses at boot (used only as the optional speed test output)`.

In "Transmit array", replace the format line with:

```
$EV,seq,ign,gear,thr,brk,steer,ldr,head,tempdC,hot,indL,indR,lampL,lampR,motor,brake,s1,s2,s3,s4,js,rpm,imA
```

and add these rows after `| 16–20 | s1 s2 s3 s4 js | 1 = pressed |`:

```markdown
| 21 | rpm | rig motor speed, rpm 0–32767 |
| 22 | imA | rig motor current, mA 0–32767 |
```

In "Build, flash, run", change `run the 30 unit tests` to `run the 39 unit tests`.

In "Flowcharts", replace the whole **Main loop** code block with:

```
          ┌──────────────┐
          │    setup()   │  pins, PWM off, speed interrupt, calibrate joystick, WiFi, send header
          └──────┬───────┘
                 ▼
   ┌──────► 100 ms passed? ──yes──► updateSpeed(): snapshot edges → rpm
   │             ▼ ◄─────────────────────┘
   │       read inputs ────────── joystick, LDR, temp, 5 switches, motor current (mV)
   │             ▼
   │       vehicleUpdate ───────── debounce → ignition → joystick % → gears
   │             │                 → indicators → headlights → motor/brake duty
   │             │                 → rig rpm and current
   │             ▼
   │       drive outputs ───────── analogWrite motor/brake, digitalWrite lamps
   │             ▼
   │      100 ms passed? ──no──┐
   │             │ yes         │
   │             ▼             │
   │     build transmit array  │
   │     Serial.print + WiFi   │
   │             ▼             │
   │      '?' received? ◄──────┘ ── yes → print header
   │             ▼
   └──────── web housekeeping
```

and add after the **Headlights** block:

```markdown
**Speed interrupt (every rising edge on GPIO4):**
```
edge ── less than SPEED_GLITCH_US since the last edge? ──yes──► ignore (noise)
  │ no
  ▼
count + 1, remember micros()
```
```

Add this section before `## Build, flash, run`:

```markdown
## DC servo rig (Semester 1 additions)

| Spec outcome | What | Where |
|---|---|---|
| O3 speed sensor | Rig disc pulses → GPIO4 interrupt → rpm every 100 ms | `src/speed_input.cpp`, `lib/ev_logic/src/speed.*` |
| O4 motor drive | Joystick throttle → PWM → TIP121 → 24 V rig motor | `drive.cpp` (unchanged), `docs/wiring.md` §3 |
| O5 current sense | Rig 1 V/A → divider → GPIO32 → mA | `sensors.cpp` `motorMilliAmps()`, `docs/wiring.md` §2 |
| O6 dashboard | rpm and amps in the transmit array, rig panel in Processing | `telemetry.*`, `EV_Dashboard.pde` |

Circuits and calculations: `../docs/wiring.md`. Testing without the rig: `../docs/home-tests.md`.
Lab checklists: `../docs/lab/`.
```

- [ ] **Step 6: Check the docs match the code**

Run (from `IES/`):

```bash
grep -n "PIN_SPEED\b\|PIN_IMOTOR\|PIN_SPEED_TEST" esp32-ev/include/pins.h
grep -n "SPEED_PULSES_PER_REV\|SPEED_GLITCH_US\|SPEED_TEST_HZ\|IMOTOR_MV_PER_A\|IMOTOR_FLOOR_MV" esp32-ev/include/config.h
grep -c "rpm" esp32-ev/README.md
```

Expected: pins 4/32/14, config values 1/20/0/500/150 as documented, and README mentions rpm (≥ 3).

- [ ] **Step 6b: Record the current-divider change in the spec.** In `docs/superpowers/specs/2026-10-15-semester1-outcomes-design.md`, replace choice 2's first two lines (`2. **Current sense:** the rig's 1 V/A output → resistor divider sized so 5 V reads about 3.0 V → ESP32 ADC` / `   (the ADC is only accurate up to about 3.1 V), plus a clamp until the real maximum is measured. Assumed one direction.`) with:

```
2. **Current sense:** the rig's 1 V/A output → 10k/10k divider (5 V → 2.5 V, inside the ESP32 ADC's most accurate
   range of about 0.15–2.45 V; readings under 150 mV count as 0 A) → ESP32 ADC, with BAT85 clamps until the real
   maximum is measured. Assumed one direction. (Changed 15 Oct during plan verification.)
```

- [ ] **Step 7: Commit**

```bash
git add docs/wiring.md docs/home-tests.md docs/lab esp32-ev/README.md docs/superpowers/specs/2026-10-15-semester1-outcomes-design.md
git commit -m "Docs: rig wiring and calculations, home tests, 22 Oct and 5 Nov lab checklists

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

- [ ] **Step 8: Explain-it checkpoint**
  1. Work out the 3.21 V and say why 10k/18k and not 10k/10k for the speed input.
  2. How do you pick RB from the current you measured on 22 Oct, and why does the shunt fallback need a smaller one?
  3. What does the flyback diode protect, and what happens without it?

---

### Task 8: A3 design poster (O2, due 22 Oct)

**Files:**
- Create: `docs/poster/poster.html`
- Create: `docs/poster/poster.pdf` (generated; force-added because `*.pdf` is ignored)

**Interfaces:**
- Consumes: circuit values from Task 7 (`docs/wiring.md`), dates from the spec.

- [ ] **Step 1: Write `docs/poster/poster.html`** (A3 portrait, print-ready, light theme for paper):

```html
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>EV Controller Poster</title>
<style>
  @page { size: A3 portrait; margin: 0; }
  :root { --ink: #12202f; --dim: #4a5a6a; --line: #c9d3dd; --accent: #0b6e4f; --amber: #b8730a; --panel: #f3f6f9; }
  * { box-sizing: border-box; }
  html, body { margin: 0; background: #fff; color: var(--ink); font-family: "Helvetica Neue", Arial, sans-serif; }
  .page { width: 297mm; height: 420mm; padding: 12mm; display: flex; flex-direction: column; gap: 5mm; }
  header { border-bottom: 1.2mm solid var(--accent); padding-bottom: 5mm; }
  h1 { font-size: 32pt; margin: 0 0 2mm; line-height: 1.05; }
  .sub { font-size: 14pt; color: var(--dim); }
  h2 { font-size: 16pt; margin: 0 0 3mm; color: var(--accent); text-transform: uppercase; letter-spacing: .04em; }
  p, li, td { font-size: 11pt; line-height: 1.4; }
  ul { margin: 0; padding-left: 5mm; }
  .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 5mm; }
  .card { background: var(--panel); border: .3mm solid var(--line); border-radius: 3mm; padding: 5mm; }
  .wide { grid-column: 1 / -1; }
  svg { width: 100%; height: auto; display: block; }
  .card:not(.wide) svg { max-height: 34mm; }
  .wide svg { max-height: 72mm; }
  svg text { font-family: "Helvetica Neue", Arial, sans-serif; fill: var(--ink); }
  .box { fill: #fff; stroke: var(--ink); stroke-width: 1.4; }
  .mcu { fill: #e3f1ec; stroke: var(--accent); stroke-width: 2; }
  .arrow { stroke: var(--ink); stroke-width: 1.6; fill: none; marker-end: url(#ah); }
  .wire { stroke: var(--ink); stroke-width: 1.6; fill: none; }
  .res { fill: #fff; stroke: var(--ink); stroke-width: 1.6; }
  table { border-collapse: collapse; width: 100%; }
  td { border-top: .3mm solid var(--line); padding: 1.5mm 2mm; vertical-align: top; }
  td:first-child { white-space: nowrap; font-weight: 600; width: 22mm; }
  .formula { font-family: Menlo, Consolas, monospace; font-size: 10.5pt; background: #fff; border: .3mm solid var(--line); border-radius: 2mm; padding: 2mm 3mm; margin-top: 2mm; }
  footer { margin-top: auto; font-size: 10pt; color: var(--dim); display: flex; justify-content: space-between; border-top: .3mm solid var(--line); padding-top: 3mm; }
</style>
</head>
<body>
<div class="page">
  <header>
    <h1>EV Controller Interface with DC Servo Rig Speed &amp; Current Sensing</h1>
    <div class="sub">Nathan Doyle · Interactive Embedded Systems · TU Dublin · ESP32-WROVER-E</div>
  </header>

  <div class="grid">
    <section class="card">
      <h2>Aim</h2>
      <p>Build the control and dashboard electronics of a small electric vehicle on an ESP32:
      read the driver's controls and sensors, drive a real 24&nbsp;V DC motor, and show
      everything live on a PC dashboard.</p>
      <ul>
        <li>Joystick throttle / brake / steering, gear switches, indicators</li>
        <li>Automatic headlights (LDR), temperature (LM35)</li>
        <li>Motor speed from the rig's slotted disc</li>
        <li>Motor current from the rig's 1&nbsp;V/A output</li>
        <li>PWM motor drive through a TIP121 Darlington</li>
      </ul>
    </section>

    <section class="card">
      <h2>Software design</h2>
      <ul>
        <li><b>Car logic in a pure C++ library</b> (no hardware calls), unit tested on a PC: 39 automated tests</li>
        <li><b>Speed:</b> interrupt counts disc edges; every 100&nbsp;ms frequency = edges ÷ time, then rpm</li>
        <li><b>Transmit array:</b> 23 values sent 10× a second as one text line over USB and WiFi</li>
        <li><b>Dashboards:</b> Processing (PC, USB serial) and a phone web page (WiFi)</li>
        <li>No <code>delay()</code>: switches, blinking and sensing never stall</li>
      </ul>
      <div class="formula">rpm = (edges ÷ seconds) × 60 ÷ slots per rev</div>
    </section>

    <section class="card wide">
      <h2>System block diagram</h2>
      <svg viewBox="0 0 900 310" role="img" aria-label="System block diagram">
        <defs><marker id="ah" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto"><path d="M0,0 L10,5 L0,10 z" fill="#12202f"/></marker></defs>
        <rect class="box" x="10" y="15"  width="210" height="40" rx="6"/><text x="115" y="40" text-anchor="middle" font-size="15">Joystick (thr / brk / steer)</text>
        <rect class="box" x="10" y="65"  width="210" height="40" rx="6"/><text x="115" y="90" text-anchor="middle" font-size="15">Switches S1–S4, start</text>
        <rect class="box" x="10" y="115" width="210" height="40" rx="6"/><text x="115" y="140" text-anchor="middle" font-size="15">LDR + LM35</text>
        <rect class="box" x="10" y="165" width="210" height="40" rx="6"/><text x="115" y="190" text-anchor="middle" font-size="15">Rig slotted disc 0–5 V</text>
        <rect class="box" x="10" y="215" width="210" height="40" rx="6"/><text x="115" y="240" text-anchor="middle" font-size="15">Rig current 1 V/A</text>
        <path class="arrow" d="M220,35 L330,140"/><path class="arrow" d="M220,85 L330,150"/><path class="arrow" d="M220,135 L330,160"/>
        <path class="arrow" d="M220,185 L330,170"/><path class="arrow" d="M220,235 L330,180"/>
        <text x="115" y="275" text-anchor="middle" font-size="12" fill="#4a5a6a">rig inputs through ÷ dividers</text>
        <rect class="mcu" x="335" y="95" width="230" height="130" rx="10"/>
        <text x="450" y="140" text-anchor="middle" font-size="19" font-weight="700">ESP32-WROVER-E</text>
        <text x="450" y="165" text-anchor="middle" font-size="14">car logic (ev_logic)</text>
        <text x="450" y="187" text-anchor="middle" font-size="14">speed interrupt · ADC · PWM</text>
        <path class="arrow" d="M565,130 L680,40"/><path class="arrow" d="M565,150 L680,110"/>
        <path class="arrow" d="M565,175 L680,195"/><path class="arrow" d="M565,200 L680,270"/>
        <rect class="box" x="680" y="15"  width="210" height="50" rx="6"/><text x="785" y="37" text-anchor="middle" font-size="15">PWM → TIP121</text><text x="785" y="56" text-anchor="middle" font-size="13" fill="#4a5a6a">24 V rig motor</text>
        <rect class="box" x="680" y="85"  width="210" height="50" rx="6"/><text x="785" y="107" text-anchor="middle" font-size="15">7-segment lamps</text><text x="785" y="126" text-anchor="middle" font-size="13" fill="#4a5a6a">head · indicators · brake</text>
        <rect class="box" x="680" y="170" width="210" height="50" rx="6"/><text x="785" y="192" text-anchor="middle" font-size="15">Processing dashboard</text><text x="785" y="211" text-anchor="middle" font-size="13" fill="#4a5a6a">PC over USB serial</text>
        <rect class="box" x="680" y="245" width="210" height="50" rx="6"/><text x="785" y="267" text-anchor="middle" font-size="15">Web dashboard</text><text x="785" y="286" text-anchor="middle" font-size="13" fill="#4a5a6a">phone over WiFi</text>
        <text x="450" y="300" text-anchor="middle" font-size="13" fill="#4a5a6a">$EV,seq,ign,gear,thr,brk,…,motor,brake,…,rpm,imA  (10 lines per second)</text>
      </svg>
    </section>

    <section class="card">
      <h2>Speed input</h2>
      <svg viewBox="0 0 400 150" role="img" aria-label="Speed input divider">
        <text x="5" y="45" font-size="13">0–5 V</text>
        <path class="wire" d="M50,40 L95,40"/><rect class="res" x="95" y="30" width="60" height="20"/><text x="125" y="22" text-anchor="middle" font-size="13">10 kΩ</text>
        <path class="wire" d="M155,40 L300,40"/><circle cx="220" cy="40" r="3.5" fill="#12202f"/>
        <text x="305" y="45" font-size="13">GPIO4</text>
        <path class="wire" d="M220,40 L220,65"/><rect class="res" x="210" y="65" width="20" height="50"/><text x="240" y="95" font-size="13">18 kΩ</text>
        <path class="wire" d="M220,115 L220,135 M205,135 L235,135"/><text x="245" y="140" font-size="13">GND</text>
      </svg>
      <div class="formula">V = 5 × 18 / (10 + 18) = 3.21 V  (≤ 3.6 V limit)</div>
    </section>

    <section class="card">
      <h2>Current input</h2>
      <svg viewBox="0 0 400 150" role="img" aria-label="Current input divider">
        <text x="5" y="45" font-size="13">1 V/A</text>
        <path class="wire" d="M50,40 L95,40"/><rect class="res" x="95" y="30" width="60" height="20"/><text x="125" y="22" text-anchor="middle" font-size="13">10 kΩ</text>
        <path class="wire" d="M155,40 L300,40"/><circle cx="210" cy="40" r="3.5" fill="#12202f"/><circle cx="270" cy="40" r="3.5" fill="#12202f"/>
        <text x="305" y="45" font-size="13">GPIO32</text>
        <path class="wire" d="M210,40 L210,65"/><rect class="res" x="200" y="65" width="20" height="50"/><text x="160" y="95" font-size="13">10 kΩ</text>
        <path class="wire" d="M270,40 L270,82 M255,82 L285,82 M255,92 L285,92 M270,92 L270,115"/><text x="292" y="92" font-size="13">1 µF</text>
        <path class="wire" d="M210,115 L210,130 L270,130 L270,115 M240,130 L240,138 M225,138 L255,138"/><text x="262" y="146" font-size="13">GND</text>
      </svg>
      <div class="formula">0.5 V per amp at the pin (0.3–4.9 A) · 5 ms filter smooths PWM ripple · BAT85 diodes clamp the pin to 0–3.6 V</div>
    </section>

    <section class="card">
      <h2>Motor drive</h2>
      <table>
        <tr><td>Switch</td><td>TIP121 Darlington, low side: ESP32 never sees 24 V</td></tr>
        <tr><td>Base</td><td>470 Ω → I<sub>B</sub> ≈ 3.6 mA, enough for 1 A (forced gain 250); smaller RB if the rig draws more</td></tr>
        <tr><td>Diode</td><td>Flyback diode across the motor absorbs the switch-off spike</td></tr>
        <tr><td>Control</td><td>PWM 0–255; gear sets the maximum; brake forces 0</td></tr>
      </table>
    </section>

    <section class="card">
      <h2>Plan</h2>
      <table>
        <tr><td>15 Oct</td><td>Scope set; existing system on real hardware</td></tr>
        <tr><td>22 Oct</td><td>Speed sensor working on the rig · this poster</td></tr>
        <tr><td>5 Nov</td><td>TIP121 drive and current sense on the rig</td></tr>
        <tr><td>Nov</td><td>Test data: speed vs duty, current vs load</td></tr>
        <tr><td>3 Dec</td><td>Report and demo</td></tr>
        <tr><td>10 Dec</td><td>Presentation and logbook</td></tr>
      </table>
    </section>
  </div>

  <footer>
    <span>Code: github.com/NatoDoyle/ies-ev-controller</span>
    <span>Out of scope this semester: CAN, H-bridge, regenerative braking, load-cell throttle</span>
  </footer>
</div>
</body>
</html>
```

- [ ] **Step 2: Make the PDF**

Run (from `IES/`):

```bash
"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" --headless --disable-gpu --no-pdf-header-footer --print-to-pdf="$PWD/docs/poster/poster.pdf" "file://$PWD/docs/poster/poster.html"
```

Expected: `... bytes written to file .../poster.pdf`.

- [ ] **Step 3: Check it is one A3 page**

Run: `grep -a -o '/Count [0-9]*' docs/poster/poster.pdf; grep -a -o '/MediaBox \[[^]]*\]' docs/poster/poster.pdf | sort -u`
Expected: `/Count 1` and `/MediaBox [0 0 841.91998 1191.12]` (A3). If it is 2 pages, the layout is too tall: shrink the gaps or SVG heights, not only the font size. Then open the PDF and look at it: nothing clipped, diagrams readable.

- [ ] **Step 4: Commit**

```bash
git add docs/poster/poster.html
git add -f docs/poster/poster.pdf
git commit -m "Add A3 design poster (HTML source and PDF)

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push
```

- [ ] **Step 5: Student rewrite (required).** The student rewrites the Aim paragraph and every bullet and table cell in their own words. Claude may check facts but doesn't write the text. Then regenerate (Step 2) and re-check (Step 3). If the lecturer's answer from H0 says AI assistance must be declared, add a footer line such as `Layout and code drafted with AI assistance; text and design decisions by the author.` Print A3 in time for the date confirmed in H0.

- [ ] **Step 6: Explain-it checkpoint (spec O2).** After the rewrite, the student presents the poster aloud in 2 minutes (timed), then answers: why 18 kΩ in the speed divider; why the TIP121 switches the low side; why the transmit array is append-only.

---

### Tasks H2–H5 (student, hardware)

**Executor: the student. Not subagent tasks.**

- [ ] **H2 (home, by Wed 21 Oct, after Tasks 5, 6 and 7):** `docs/home-tests.md` parts A and B (build and test both dividers). Then the bad-line check (Review Focus 5): flash `o1-baseline` (21 fields) and run the new Processing sketch. The status bar must show `BAD LINES` rising about 10 per second while the values stay frozen. Flash `main` again: the counter stops rising. Logbook entry.
- [ ] **H3 (lab, Thu 22 Oct):** `docs/lab/2026-10-22-speed-sensor.md`. Commit the measured `SPEED_PULSES_PER_REV` and the logbook entry the same day.
- [ ] **H4 (home, reading week):** `docs/home-tests.md` part C. Build the TIP121 stage (RB from the 22 Oct current, heatsink, star ground). Logbook entries.
- [ ] **H5 (lab, Thu 5 Nov):** `docs/lab/2026-11-05-rig-drive-current.md`. Commit results and the logbook entry.

## Order and dates

| When | Task |
|---|---|
| Thu 15 Oct | Task 1 → H0 and H1 can start |
| by Fri 16 Oct | H0 (manual and questions to the technician, poster date and AI policy to the lecturer) |
| 16–18 Oct | Tasks 2, 3, 4, 5 (firmware ready for H2) |
| 18–19 Oct | Tasks 6, 7, 8 (dashboard, docs, poster) |
| by Wed 21 Oct | H1 done, H2 done, poster printed |
| Thu 22 Oct | H3 |
| Reading week | H4 |
| Thu 5 Nov | H5 |

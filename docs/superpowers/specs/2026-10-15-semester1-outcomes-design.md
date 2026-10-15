# Semester 1 outcomes: IES EV Controller (to 10 Dec 2026)

## Context
One project idea, two codebases:
- **`IES/esp32-ev`** (ESP32-WROVER-E) + **Processing dashboard** (`IES/dashboard/EV_Dashboard`) + web dashboard.
  This is the college-module deliverable. Goal: **the minimum that reliably gets an A**, with the
  user able to **explain every line**. The lecturer has approved the ESP32 port in place of the MSP430.
- **`fs-master-control-unit`** (ESP32-C3) is the version being developed for Formula Student next year.
  **Paused until after 10 Dec.** No outcomes this semester.

Why this document: the written IES brief (10 learning outcomes, "Project 2025") is out of date.
The real scope was given on the whiteboard (`IMG_3985.png`): the old brief **plus a speed sensor,
DC servo rig testing and motor current sensing**. There is no rubric.

Starting state: the esp32-ev code builds and passes unit tests but **has never run on real hardware**.
The project is not in git. The logbook (digital, printed if needed) has not been started.

## Decisions (from the 15 Oct outcomes interview)
| Topic | Decision |
|---|---|
| Graded build | Extend `IES/esp32-ev`; FS MCU separate and paused |
| Semester 1 additions | Speed sensor, DC servo rig (open-loop), motor current sense |
| Not this semester | CAN, H-bridge, regen braking, load-cell throttle, FFT and freq-to-voltage speed methods |
| Speed method | Simplest: timer/interrupt pulse timing |
| Speed source | The rig's own slotted disc |
| Rig goal (5 Nov) | Open-loop: throttle → ESP32 PWM → rig motor; ESP32 measures speed + armature current; dashboard shows them |
| Learning bar | You can explain every line (Claude may write code, with explain-it checkpoints) |
| Hardware | WROVER + kit at home; rig **lab only, Thursdays, none in reading week** |
| Rig facts known | Speed output is a 0–5 V square wave; 24 V DC motor; armature current out 1 V/A (max voltage not yet known); DVM ±19.9 V; manual not online |
| Motor drive | Your own TIP121 low-side stage switches the rig's 24 V motor |
| Poster | A3, design/plan poster (no results needed), plan for **22 Oct** |
| Logbook | **Digital**, printed if needed; not started |
| Report | No spec given |
| Git | Public GitHub repo, noreply identity |

## Choices made on your behalf (approved 15 Oct; revisit if wrong)
1. **Motor drive details:** TIP121 low-side + flyback diode from a 24 V supply (matches the whiteboard,
   testable at home on the kit motor at 5 V, your circuit to explain). Fallback: the rig's built-in amplifier input.
2. **Current sense:** the rig's 1 V/A output → resistor divider sized so 5 V reads about 3.0 V → ESP32 ADC
   (the ADC is only accurate up to about 3.1 V), plus a clamp until the real maximum is measured. Assumed one direction.
   Fallback: a shunt resistor in the TIP12x emitter.
3. **Speed input:** the rig's 0–5 V square wave → resistor divider (5 V → about 3.3 V) → an ESP32 interrupt pin.
   Confirm with a scope on 22 Oct before connecting.
4. **Speed code:** port the tested speed estimator from FS MCU (`lib/mcu_core/src/speed.*`) into
   `lib/ev_logic` (same author, copied, FS MCU not touched). Unit: motor-shaft rpm (slots per rev counted in the lab).
5. **Throttle/brake stay on the joystick** as now; brake still forces motor duty to 0.
6. **Graded dashboard: Processing.** The web dashboard keeps working; adding the new fields there is a stretch goal.
7. **Report default:** a 10–15 page technical report (intro, system design, circuits, firmware, testing with real data, conclusions).
8. **Repo:** root at `IES/`; the lecturer's PDFs and the whiteboard/rig photos are git-ignored (course material).
9. **Logbook:** `IES/docs/logbook.md` in the repo (each commit timestamps the entries, which is evidence),
   exported to PDF for printing. Diagrams as images in `IES/docs/logbook/`. Past weeks go in a clearly marked
   *retrospective* section; real-time entries start today. You write the entries; Claude supplies material
   (flowcharts, circuit diagrams, calculations, test data) for you to put in your own words.
10. **"An A" means** (no rubric exists): every requirement works live on demo day, the report proves it with
    measured data, the logbook is complete, and you can answer "how does this work?" on any part.

## Outcomes (each one: done-when, plus an explain check)

| # | Outcome | Due | Done when | Explain check |
|---|---|---|---|---|
| O0 | Project under version control | 16 Oct | `IES/` is a public GitHub repo; first commit = current state; PDFs/photos ignored | — |
| O1 | Base build runs on real hardware | 21 Oct (home) | Every existing feature verified on the wired WROVER against a written checklist: 5 switches, joystick X/Y/SEL, LDR headlights, LM35 temp, indicators, gears, motor PWM, brake light, Processing dashboard | You can trace one switch press from pin to dashboard |
| O2 | A3 poster | 22 Oct | Print-ready A3 PDF: problem, system block diagram (inputs → ESP32 → drive/rig → dashboard), circuits, plan to Dec | You can present it in 2 min |
| O3 | Speed sensor on the rig | 22 Oct (lab) | Rig disc pulses (0–5 V) → divider → ESP32 → rpm on Processing; agrees with the rig's RPM display within ±5 % at 3 speeds. Pulse level confirmed and slot count measured, both logged | You can explain ISR timing → rpm maths |
| O4 | Rig open-loop drive | 5 Nov (lab) | Joystick throttle sets PWM → TIP12x → rig motor; 0 % = stopped; brake = 0 duty. Tested first at home on the kit motor | You can explain the drive circuit and base-resistor maths |
| O5 | Motor current sense | 5 Nov (lab) | Rig 1 V/A out → ESP32 → amps on Processing; agrees with a meter within ±10 % at 2 loads | You can explain the divider values |
| O6 | Dashboard shows new data | 5 Nov | Processing shows rpm and motor current live, alongside everything from O1; the serial line format is documented | You can explain the transmit array fields |
| O7 | Test data for the report | 26 Nov (labs 12/19/26 Nov) | Logged runs: speed vs duty curve, current vs load, plus the O3/O5 accuracy checks, saved as CSV | You can explain what the curves show |
| O8 | Report | 3 Dec | Technical report covering O1–O7 with real data, circuits, flowcharts, a full pin map, and limitations | You wrote/understand every section |
| O9 | Demo + talk | 3 Dec | Scripted demo run: every feature in a fixed order, a fallback plan if the rig misbehaves | Rehearsed at least once end to end |
| O10 | Presentation | 10 Dec | 5-minute slide deck, rehearsed to time | — |
| O11 | Digital logbook | 10 Dec (weekly) | `docs/logbook.md` with weekly dated entries from 15 Oct, a retrospective section for earlier weeks, and the design work: flowcharts, circuit diagrams, calculations, test results; exports cleanly to a printable PDF | It's in your own words and you can talk through any entry |

Stretch, only once O0–O11 are on track: web dashboard shows rpm/current; brake-plausibility logic from FS MCU.

## Week plan
- **15–21 Oct (home):** O0, O1 (bring-up), O2 (poster), O3 prep: speed code + protected input, tested with
  the ESP32 generating its own test pulses. Start O11. **Ask the technician for the 33-100 manual.**
- **Thu 22 Oct (lab):** poster in; O3 on the rig; scope the encoder output + current output; count slots.
- **Reading week (home):** O4 drive stage + O5 conditioning + O6 dashboard, all proven on the kit motor.
- **Thu 5 Nov (lab):** O4 + O5 on the rig.
- **12 / 19 / 26 Nov (labs):** O7 data runs, fixes; report drafting at home.
- **3 Dec:** O8 + O9. **10 Dec:** O10 + O11.

## Risks
- **Only two rig sessions before the milestones**, so everything must work first time → home testing with simulated signals, and a written lab-session checklist for each visit.
- **A 5 V signal straight into a 3.3 V pin** can damage the ESP32 → every rig signal goes through a divider; check the level with a scope first.
- **Base build never run on hardware** → O1 first; any bring-up bug eats into the 22 Oct prep.
- **Poster date uncertain** → planned for 22 Oct; confirm this week.
- **24 V motor supply on the same bench** → common ground with the rig, never wire with power on, power-up order written down.

## Still unknown (to find out, not decide)
Poster date (22 Oct or 5 Nov) · slots per rev · current output's maximum voltage · rig motor terminal access · component weights of report/demo.

## Verification (of the outcomes as a whole)
- Each outcome's "done when" is checked on real hardware or as a real artifact (no "should work").
- Lab days: run the per-session checklist; log measurements in the logbook the same day.
- Firmware changes: `pio test -e native` in `IES/esp32-ev` stays green; new logic (speed, current scaling) gets unit tests.

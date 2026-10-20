# Suggested improvements (not applied)

`src/main.cpp` is a straight port of `firmware.cpp`: the logic is the same as on the MSP430.
These are ideas for improving it, roughly most useful first. They are optional. Make one
change, test it on the board, then move on to the next. Where one idea depends on another,
it says so.

## 1. Make the motor speed follow the joystick

**Now:** `analogWrite` works in 0–255, but `JS_U_D` is 551–1023 whenever the motor runs, so
the motor is always fully on (anything at or above 255 is fully on). On the MSP430 this was
an accidental ramp caused by the PWM timer overflowing (see the README).

**Change:** add one line at the end of `setup()` so `analogWrite` works in 0–1023, the same
range as the joystick reading:

```cpp
  analogWriteResolution(Motor, 10);  // analogWrite takes 0-1023 for every PWM pin
```

Called before the first `analogWrite`, it sets 10 bits for every PWM pin (the motor and the
brake light). The motor then runs at about 54 % at 551, rising to about 95–100 % at full
stick. The brake light becomes 0–49 % (dimmer when braking lightly), so do idea 2 with it.

## 2. Brake light gets dimmer the harder you brake

**Now:** `analogWrite(BL, JS_U_D)` writes the raw joystick value. Pushing down makes
`JS_U_D` smaller, so below 255 the brake light gets dimmer as you brake harder, and it's off
at full brake.

**Change:** brightness grows as the stick goes down from 500 towards 0. Replace the
`if (JS_U_D < 500) { ... } else { ... }` block with:

```cpp
  if (JS_U_D < 500) {
    analogWrite(BL, (500 - JS_U_D) * 255 / 500);   // 0 at 500, full (255) at 0
  } else {
    analogWrite(BL, 0);
  }
```

(If you did idea 1, use `1023` instead of `255`.)

## 3. `delay(250)` freezes everything, and holding a switch repeats it

**Now:** each switch press runs `delay(250)`. For that quarter of a second nothing else
happens: the motor, lights and serial output all stop updating. Holding S1 also keeps
changing gear every 250 ms, so it's easy to overshoot.

**Change:** remember the switch's last state and act only on the press (HIGH → LOW), using
`millis()` instead of `delay()`:

```cpp
bool lastS1 = HIGH;             // global (above setup): S1 state last time round the loop
unsigned long lastS1Change = 0; // global: when S1 last changed

// in loop(), replacing the existing if (digitalRead(S1) == LOW) { ... } block:
bool s1 = digitalRead(S1);
if (s1 != lastS1 && millis() - lastS1Change > 30) {  // changed, and not contact bounce
  lastS1Change = millis();
  if (s1 == LOW) {              // just pressed (not held)
    gearPos++;
    if (gearPos > 4) gearPos = 0;   // same wrap as now (idea 4 changes this)
  }
  lastS1 = s1;
}
```

Do the same for S3. The S2 and S4 indicator branches still use `delay(250)`. Once they're
changed too (idea 5), there's no `delay()` left.

## 4. Gear up wraps round but gear down stops

**Now:** S1 goes 4 → 0 (wraps round to neutral), while S3 stops at 0. Going from top gear
straight to neutral while driving is surprising.

**Change:** stop at 4 as well: `if (gearPos > 4) { gearPos = 4; }`.

## 5. Indicators are solid only while the switch is held

**Now:** the indicator lights only while S2/S4 is held down, and doesn't flash.

**Change:** a press turns the indicator on, a second press turns it off, and while it's on
it flashes about 1.5 times a second. Detect the press the same way as idea 3, then:

```cpp
bool leftOn = false;            // global: left indicator switched on?
// on an S4 press:  leftOn = !leftOn;
// every loop, replacing the existing S4 if/else block:
bool flash = (millis() / 333) % 2 == 0;           // on for 333 ms, off for 333 ms
digitalWrite(LIL, (leftOn && flash) ? HIGH : LOW);
```

## 6. Serial output floods and the labels repeat every loop

**Now:** every pass of `loop()` prints "Gear Position", "Temp Celsius" and the values: about
37 characters, which take about 40 ms at 9600 baud. So the serial port sets the loop speed,
and switches and the joystick respond more slowly. The headings also repeat on every line.

**Change:** print the headings once in `setup()`, and the values every 500 ms:

```cpp
unsigned long lastPrint = 0;    // global
// in loop(), replacing the four value prints:
if (millis() - lastPrint >= 500) {
  lastPrint = millis();
  Serial.print(gearPos);
  Serial.print("\t");
  Serial.println(Temp);
}
```

## 7. A transmit array for a PC dashboard (learning outcomes 3–5)

**Now:** the output is text for a person to read, and only shows the gear and temperature.

**Change:** collect every value in one array and send it as one comma-separated line, which
a Processing dashboard can split back into numbers:

```cpp
int tx[10] = {gearPos, JS_U_D, JS_L_R, lightSensor, tempSensor,
              digitalRead(S1) == LOW, digitalRead(S2) == LOW,
              digitalRead(S3) == LOW, digitalRead(S4) == LOW, JS_Centre};
Serial.print("$EV");
for (int i = 0; i < 10; i++) { Serial.print(","); Serial.print(tx[i]); }
Serial.println();
```

(The `EV_Dashboard.pde` already in the repo expects the bigger project's 21-field line, so
it would need its field list changed to match.)

## 8. The gear doesn't affect the motor

**Now:** `gearPos` is shown, but the motor only depends on the joystick.

**Change (needs idea 1):** let each gear cap the top speed: neutral = 0, gears 1–4 =
25/50/75/100 %. Replace the motor `if/else` block with:

```cpp
  const int JS_MAX = 1023;  // set to your measured JS_U_D at full stick (often ~960-1000 on the ESP32)
  int maxDuty[5] = {0, 256, 512, 767, 1023};       // N,1,2,3,4
  if (JS_U_D > 550) {
    long duty = (long)(JS_U_D - 550) * maxDuty[gearPos] / (JS_MAX - 550);
    if (duty > maxDuty[gearPos]) duty = maxDuty[gearPos];
    analogWrite(Motor, duty);
  } else {
    analogWrite(Motor, 0);
  }
```

## 9. Unused code

- `displayValue[4]` is never used. Remove it, or use it if it was meant for the 7-segment display.
- `JS_Centre` (the joystick button) and `JS_L_R` (steering) are read but never used.
  The button could be an ignition (start/stop) toggle that the motor needs before it runs.
- The button is a switch, so `digitalRead()` with `pinMode(JS_CENTRE_PIN, INPUT_PULLUP)`
  suits it better than `analogRead`.

## 10. Steadier temperature

**Now:** one reading per loop, so the temperature jitters by a few tenths of a degree.

**Change:** replace the line `float Temp = tempSensor * 0.322;` with these. They average 8
readings and use the millivolts directly (LM35: 10 mV per °C):

```cpp
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogReadMilliVolts(TEMP_PIN);
  float Temp = sum / 80.0;    // average mV / 10 -> °C
```

## 11. Not in this firmware yet: the DC servo rig

The semester's extra requirements (the speed sensor from the rig's slotted disc, due 22 Oct,
and the rig motor drive and current sensing, due 5 Nov) aren't part of the MSP430 code, so
they aren't here. When you're ready to add them: the speed sensor is an interrupt that
counts pulses on one pin, and the current is one more `analogRead`. Both can be added to
this file without restructuring it.

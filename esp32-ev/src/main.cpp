/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       2.0

  Hardware:
  ESP32-WROVER-E DevKit (DevKitC-VE) with:
   - Analog joystick (VER = throttle/brake, HOR = steering, SEL = start/stop)
   - LDR + 10k divider (automatic headlights)
   - DC motor driven by a TIP120 Darlington (PWM speed control)
   - 7-segment display used as lamps: a = headlights, b+c = right indicator,
     d = brake light, e+f = left indicator
   - Push switches S1 (gear up), S3 (gear down), S4 (left), S2 (right)
  Pin map in include/pins.h, settings in include/config.h.

  Description:
  EV Controller Interface - ported from the MSP430 (Energia) version.
  Every loop:  read inputs -> run the car logic -> drive outputs
  Every 100 ms: build the TRANSMIT ARRAY and send it as one line
                "$EV,..." over USB serial (Processing dashboard) and WiFi
                (web dashboard).
  The car logic itself lives in lib/ev_logic so it can be unit tested on a PC.
  No delay() is used, so switches and blinking never stall.

  History:
  V1.0 :- 20250924
  MSP430 version (firmware.cpp).
  V2.0 :- 20261008
  Ported to the ESP32-WROVER-E. Edge-detected debounce (no auto-repeat),
  gears clamp instead of wrapping, PWM values scaled to 0..255, joystick
  button read digitally, indicators latch and blink, header sent once,
  WiFi web dashboard added.

  Configuration:
  USB cable to the PC, 115200 baud. Close the PlatformIO serial monitor
  before running the Processing dashboard (only one program can use the port).
  Send '?' over serial to get the "#EV" header line again.
*/
#include <Arduino.h>                  // Arduino API (pinMode, analogRead, ...)
#include "pins.h"                     // pin numbers
#include "config.h"                   // tunable settings
#include "vehicle.h"                  // car logic (lib/ev_logic)
#include "telemetry.h"                // transmit array (lib/ev_logic)
#include "web_dashboard.h"            // WiFi web dashboard

static VehicleState car;              // everything the car knows
static uint32_t lastTx = 0;           // time of the last transmit
static uint16_t txSeq = 0;            // transmit counter

// Read one analog pin several times and return the average (smooths noise).
static int readAveraged(uint8_t pin, int samples) {
  long sum = 0;                       // running total
  for (int i = 0; i < samples; i++) { // take several readings
    sum += analogRead(pin);           // 12 bit reading 0..4095
  }
  return (int)(sum / samples);        // average
}

// Copy the #defines from config.h into the settings used by the car logic.
static VehicleConfig makeConfig() {
  VehicleConfig cfg;                  // starts with the defaults
  const uint8_t gears[] = GEAR_MAX_DUTY;  // N,1,2,3,4 duty caps
  for (int g = 0; g <= GEAR_TOP; g++) cfg.gearMaxDuty[g] = gears[g];  // copy each gear
  cfg.debounceMs      = DEBOUNCE_MS;       // switch settle time
  cfg.jsDead          = JS_DEADZONE;       // joystick deadzone
  cfg.jsCentreDefault = JS_CENTRE_DEFAULT; // fallback centre
  cfg.jsCentreMin     = JS_CENTRE_MIN;     // centre limits
  cfg.jsCentreMax     = JS_CENTRE_MAX;     //  "
  cfg.throttleInvert  = THROTTLE_INVERT;   // throttle direction
  cfg.steerInvert     = STEER_INVERT;      // steering direction
  cfg.motorMinDuty    = MOTOR_MIN_DUTY;    // motor stall limit
  cfg.tailDuty        = TAIL_DUTY;         // tail light level
  cfg.ldrOn           = LDR_ON;            // dark threshold
  cfg.ldrOff          = LDR_OFF;           // light threshold
  cfg.ldrHoldMs       = LDR_HOLD_MS;       // headlight hold time
  cfg.blinkHalfMs     = BLINK_HALF_MS;     // indicator speed
  cfg.tempEnabled     = TEMP_ENABLED;      // temperature sensor fitted?
  cfg.tempIsTmp36     = TEMP_IS_TMP36;     // sensor type
  cfg.overheatDeciC   = OVERHEAT_DECI_C;   // overheat limit
  return cfg;                         // finished settings
}

// Read every input into one Inputs snapshot (switches: true = pressed).
static Inputs readInputs() {
  Inputs in;                          // snapshot
  in.ver = readAveraged(PIN_JS_VER, ADC_SAMPLES);  // throttle/brake axis
  in.hor = readAveraged(PIN_JS_HOR, ADC_SAMPLES);  // steering axis
  in.ldr = readAveraged(PIN_LDR, ADC_SAMPLES);     // light sensor
  in.tempMv = TEMP_ENABLED ? analogReadMilliVolts(PIN_TEMP) : 0;  // calibrated millivolts
  in.s1 = digitalRead(PIN_S1) == LOW;      // pull-up: LOW = pressed
  in.s2 = digitalRead(PIN_S2) == LOW;      //  "
  in.s3 = digitalRead(PIN_S3) == LOW;      //  "
  in.s4 = digitalRead(PIN_S4) == LOW;      //  "
  in.js = digitalRead(PIN_JS_SEL) == LOW;  //  "
  return in;                          // done
}

// Set every output from the car state.
static void driveOutputs() {
  analogWrite(PIN_MOTOR, car.motor);  // motor speed 0..255
  analogWrite(PIN_BRAKE, car.brake);  // brake light brightness 0..255
  digitalWrite(PIN_HEAD, car.headOn ? HIGH : LOW);             // headlights (segment a)
  digitalWrite(PIN_IND_L, car.ind.leftLamp() ? HIGH : LOW);    // left indicator (e+f)
  digitalWrite(PIN_IND_R, car.ind.rightLamp() ? HIGH : LOW);   // right indicator (b+c)
}

// Build the transmit array and send it to both dashboards.
static void transmit() {
  int16_t tx[TX_COUNT];               // THE transmit array
  char line[TX_LINE_MAX];             // text version of it
  buildTxArray(car, txSeq++, tx);     // fill the array from the car state
  if (formatTxLine(tx, line, sizeof line) > 0) {  // "$EV,...\n"
    Serial.print(line);               // to the PC (Processing)
    webPush(line);                    // to phones (web dashboard)
  }
}

// PC -> ESP32: '?' asks for the header line again (two way serial).
static void handleSerialCommand() {
  while (Serial.available() > 0) {    // something was sent from the PC
    char c = (char)Serial.read();     // read one character
    if (c == '?') {                   // header request
      Serial.print(txHeader());       // "#EV,seq,ign,..."
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);          // USB serial to the PC

  pinMode(PIN_S1, INPUT_PULLUP);      // switches to GND, internal pull-up
  pinMode(PIN_S2, INPUT_PULLUP);      //  "
  pinMode(PIN_S3, INPUT_PULLUP);      //  "
  pinMode(PIN_S4, INPUT_PULLUP);      //  "
  pinMode(PIN_JS_SEL, INPUT_PULLUP);  // joystick button (was analogRead on the MSP430)

  pinMode(PIN_HEAD, OUTPUT);          // headlights
  pinMode(PIN_IND_L, OUTPUT);         // left indicator
  pinMode(PIN_IND_R, OUTPUT);         // right indicator
  digitalWrite(PIN_HEAD, LOW);        // all lamps off
  digitalWrite(PIN_IND_L, LOW);       //  "
  digitalWrite(PIN_IND_R, LOW);       //  "

  // NOTE: no pinMode() on the PWM pins - on the ESP32 pinMode() detaches PWM.
  analogWrite(PIN_MOTOR, 0);          // attach PWM with the motor OFF
  analogWrite(PIN_BRAKE, 0);          // attach PWM with the brake light off

  int verCentre = readAveraged(PIN_JS_VER, CAL_SAMPLES);  // joystick at rest (don't touch it at power up!)
  int horCentre = readAveraged(PIN_JS_HOR, CAL_SAMPLES);  //  "
  vehicleInit(car, makeConfig(), verCentre, horCentre);   // set up the car logic

  if (car.verCal.calFailed || car.horCal.calFailed) {     // stick was held at power up
    Serial.println("# Joystick centre out of range - using default");  // '#' = not data
  }
  if (WEB_ENABLED) webBegin();        // start the WiFi dashboard
  Serial.print(txHeader());           // field names, sent once
}

void loop() {
  uint32_t now = millis();            // time now (ms since power up)
  Inputs in = readInputs();           // 1 - read every input
  vehicleUpdate(car, in, now);        // 2 - work out what the car should do
  driveOutputs();                     // 3 - set motor, lights and indicators
  if (now - lastTx >= TX_PERIOD_MS) { // 4 - time to send?
    lastTx = now;                     // remember when
    transmit();                       // send the transmit array
  }
  handleSerialCommand();              // 5 - check for '?' from the PC
  if (WEB_ENABLED) webLoop();         // 6 - web housekeeping
}

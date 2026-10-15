/*
  Written         08/10/2026
  by              Nathan Doyle
  Student No.     X0000000

  Revision:       1.0

  Hardware:
  ESP32-WROVER-E DevKit running the EV Controller firmware, USB cable to PC.

  Description:
  PC dashboard GUI for the EV Controller Interface project.
  The ESP32 sends its "transmit array" over USB serial, one line at a time,
  in the form  $EV,<21 comma-separated integers>
  This sketch reads each line, checks it is valid, splits it into numbers
  and draws them as a car-style dashboard:
    - motor power gauge (motor PWM duty shown as a percentage)
    - throttle and brake bars, plus a brake-light glow
    - gear indicator (N, 1, 2, 3, 4)
    - steering wheel that turns with the steering input
    - left / right indicator arrows
    - headlight icon and LDR light-level bar
    - ignition badge and temperature read-out
    - lamps for switches S1 S2 S3 S4 and the joystick button
    - a 7-segment digit that copies the physical lamps
  Two-way serial: pressing H (and once after the port opens) sends '?'
  to the ESP32, which answers by printing its "#EV" header line again.
  Demo mode makes up realistic data so the GUI works without the board.

  History:
  V1.0 :- 20261008
  First version.

  Configuration:
  Serial settings: 115200 baud, 8 data bits, no parity, 1 stop bit.
  Close the PlatformIO serial monitor first - only one program can use
  the port at a time.
  macOS port names: /dev/cu.usbserial-*, /dev/cu.SLAB_USBtoUART,
  /dev/cu.wchusbserial*, /dev/cu.usbmodem*.  Windows: COMx.
  Keys:  P = next port   D = demo mode on/off   H = ask for header
  Libraries: core Processing 4 and processing.serial only.
*/

import processing.serial.*;                    // Processing serial library for the USB port

// ---------------- Serial settings ----------------
final int BAUD            = 115200;            // must match Serial.begin() in the firmware
final int NUM_FIELDS      = 21;                // integers after "$EV" on each line
final int STALE_MS        = 1500;              // no line for this long means data is stale
final int HEADER_DELAY_MS = 2000;              // wait for the ESP32 to reboot before sending '?'
final int DEMO_PERIOD_MS  = 100;               // demo data update period (100 ms = 10 Hz)
final int TEMP_NOT_FITTED = -999;              // firmware sends this when no temp sensor is fitted

// ---------------- Transmit array indexes (mirror the firmware) ----------------
final int F_SEQ   = 0;                         // sequence number, counts up every line
final int F_IGN   = 1;                         // ignition 0 = off, 1 = on
final int F_GEAR  = 2;                         // gear 0 = N, 1..4
final int F_THR   = 3;                         // throttle 0..100 %
final int F_BRK   = 4;                         // brake pedal 0..100 %
final int F_STEER = 5;                         // steering -100..100, negative = left
final int F_LDR   = 6;                         // LDR raw 0..4095, higher = darker
final int F_HEAD  = 7;                         // headlights 0 = off, 1 = on
final int F_TEMP  = 8;                         // temperature in tenths of a degree C
final int F_HOT   = 9;                         // over-temperature flag 0/1
final int F_INDL  = 10;                        // left indicator switched on
final int F_INDR  = 11;                        // right indicator switched on
final int F_LAMPL = 12;                        // left indicator lamp lit right now (blinks)
final int F_LAMPR = 13;                        // right indicator lamp lit right now (blinks)
final int F_MOTOR = 14;                        // motor PWM duty 0..255
final int F_BRAKE = 15;                        // brake light PWM duty 0..255
final int F_S1    = 16;                        // switch S1 pressed (gear up)
final int F_S2    = 17;                        // switch S2 pressed (right indicator)
final int F_S3    = 18;                        // switch S3 pressed (gear down)
final int F_S4    = 19;                        // switch S4 pressed (left indicator)
final int F_JS    = 20;                        // joystick button pressed (start / stop)

// ---------------- Port name patterns ----------------
final String[] PORT_PATTERNS = {               // parts of port names that look like an ESP32
  "cu.usbserial",                              // macOS CP210x / CH340 (common name)
  "cu.SLAB_USBtoUART",                         // macOS Silicon Labs CP210x driver
  "cu.wchusbserial",                           // macOS WCH CH340 driver
  "cu.usbmodem",                               // macOS native USB boards
  "ttyUSB",                                    // Linux USB-serial adapters
  "ttyACM"                                     // Linux native USB boards
};                                             // end of pattern list

// ---------------- Serial state ----------------
Serial port = null;                            // the open serial port (null = none open)
String portName = "";                          // name of the open port, for the status bar
ArrayList<String> candidates = new ArrayList<String>();  // ports that look like an ESP32
int portIndex = -1;                            // which candidate is in use (-1 = none yet)
boolean demoMode = false;                      // true = show made-up demo data
volatile int[] data = blankData();             // latest transmit array (whole array swapped at once)
volatile int lastRxMillis = 0;                 // millis() when the last good line arrived
int headerRequestAt = 0;                       // millis() at which to send '?' (0 = nothing pending)

// ---------------- Demo state ----------------
int lastDemoMillis = 0;                        // millis() of the last demo update
int demoSeq = 0;                               // demo sequence counter

// ---------------- Colour palette ----------------
color cBg;                                     // window background
color cPanel;                                  // panel background
color cEdge;                                   // panel outline
color cText;                                   // main text
color cDim;                                    // labels and secondary text
color cOff;                                    // unlit lamps and empty tracks
color cGreen;                                  // OK / indicators / pressed switches
color cAmber;                                  // warnings, neutral gear, demo
color cRed;                                    // brake, hot, errors
color cBlue;                                   // headlights


void settings() {                              // runs before setup(), sets window options
  size(1000, 620);                             // window size in pixels
  smooth(8);                                   // anti-aliased (smooth) edges
}                                              // end of settings()


void setup() {                                 // runs once at start-up
  surface.setTitle("EV Controller Dashboard"); // text on the window title bar
  frameRate(30);                               // redraw 30 times a second
  initColours();                               // fill in the colour palette
  scanPorts();                                 // look for an ESP32 serial port
  if (candidates.size() > 0) {                 // at least one likely port found?
    openPort(0);                               // try to open the first one
  } else {                                     // no likely port found
    println("No ESP32 port found - starting in demo mode");  // tell the user
    demoMode = true;                           // fall back to demo data
  }                                            // end of port check
}                                              // end of setup()


void draw() {                                  // runs every frame
  if (demoMode) {                              // demo mode active?
    updateDemo();                              // make new demo data at 10 Hz
  }                                            // end of demo check
  checkHeaderRequest();                        // send '?' once the ESP32 has booted
  int[] d = data;                              // take ONE snapshot so the whole frame is consistent
  background(cBg);                             // clear the window
  drawPanels();                                // panel backgrounds and titles
  drawTitleBar(d);                             // heading and ignition badge
  drawPedals(d);                               // throttle bar, brake bar, brake light
  drawSpeedo(d);                               // motor power gauge
  drawGear(d);                                 // big gear letter in the gauge
  drawIndicators(d);                           // left / right arrows
  drawSteering(d);                             // steering wheel
  drawHeadlights(d);                           // headlight icon and LDR bar
  drawSwitches(d);                             // S1..S4 and JS lamps
  drawSevenSeg(d);                             // 7-segment lamp mirror
  drawTemperature(d);                          // temperature read-out
  drawStatusBar(d);                            // port / demo / stale, seq, key help
}                                              // end of draw()


// =====================================================================
//  Set-up helpers
// =====================================================================

void initColours() {                           // sets every palette colour
  cBg    = color(14, 17, 23);                  // near-black blue background
  cPanel = color(24, 29, 38);                  // slightly lighter panels
  cEdge  = color(48, 56, 70);                  // subtle panel outline
  cText  = color(230, 236, 243);               // almost-white text
  cDim   = color(130, 142, 158);               // grey label text
  cOff   = color(45, 52, 64);                  // unlit / empty
  cGreen = color(46, 204, 113);                // bright green
  cAmber = color(255, 176, 32);                // amber
  cRed   = color(235, 64, 52);                 // red
  cBlue  = color(80, 170, 255);                // headlight blue
}                                              // end of initColours()


int[] blankData() {                            // returns an empty transmit array
  int[] d = new int[NUM_FIELDS];               // all fields start at 0
  d[F_TEMP] = TEMP_NOT_FITTED;                 // show "--" until real data arrives
  return d;                                    // hand back the new array
}                                              // end of blankData()


boolean on(int v) {                            // true when a 0/1 flag is set
  return v != 0;                               // any non-zero value counts as on
}                                              // end of on()


// =====================================================================
//  Serial port handling
// =====================================================================

void scanPorts() {                             // builds the list of likely ESP32 ports
  candidates.clear();                          // forget the old list
  String[] all = new String[0];                // empty list in case Serial.list() fails
  try {                                        // listing ports can fail on some PCs
    all = Serial.list();                       // ask the OS for every serial port
  } catch (Exception e) {                      // listing failed
    println("Could not list serial ports: " + e.getMessage());  // report the problem
  }                                            // end of try
  println("Serial ports found:");              // console heading
  printArray(all);                             // print every port with its index
  for (int i = 0; i < all.length; i++) {       // check each port name
    if (isWantedPort(all[i])) {                // looks like an ESP32?
      candidates.add(all[i]);                  // keep it
    }                                          // end of name check
  }                                            // end of loop
  println("Candidate ESP32 ports: " + candidates);  // show the short list
}                                              // end of scanPorts()


boolean isWantedPort(String name) {            // true if the name looks like an ESP32 port
  if (name.startsWith("/dev/tty.")) {          // macOS tty.* ports block on open
    return false;                              // use the cu.* version instead
  }                                            // end of tty check
  if (name.toLowerCase().contains("bluetooth")) {  // Bluetooth serial ports
    return false;                              // never the ESP32 USB cable
  }                                            // end of Bluetooth check
  if (name.startsWith("COM")) {                // Windows port (COM3, COM4, ...)
    return true;                               // accept it
  }                                            // end of Windows check
  for (int i = 0; i < PORT_PATTERNS.length; i++) {  // check each known pattern
    if (name.contains(PORT_PATTERNS[i])) {     // pattern found in the name?
      return true;                             // accept it
    }                                          // end of pattern check
  }                                            // end of loop
  return false;                                // nothing matched
}                                              // end of isWantedPort()


void openPort(int idx) {                       // opens candidate number idx
  closePort();                                 // close any port already open
  if (idx < 0 || idx >= candidates.size()) {   // index out of range?
    demoMode = true;                           // nothing to open, use demo data
    return;                                    // stop here
  }                                            // end of range check
  String name = candidates.get(idx);           // port name to open
  portIndex = idx;                             // remember it so P moves to the next one
  try {                                        // opening can throw (port busy, unplugged ...)
    port = new Serial(this, name, BAUD);       // open the port at 115200 baud
    port.clear();                              // throw away anything already buffered
    port.bufferUntil('\n');                    // call serialEvent() once per full line
    portName = name;                           // show this name in the status bar
    demoMode = false;                          // real data from now on
    data = blankData();                        // clear old values off the screen
    lastRxMillis = millis();                   // grace period before showing STALE
    headerRequestAt = millis() + HEADER_DELAY_MS;  // ESP32 resets on open, ask for header later
    println("Opened " + name + " at " + BAUD + " baud");  // console message
  } catch (Exception e) {                      // port could not be opened
    println("Could not open " + name + ": " + e.getMessage());  // report why
    port = null;                               // no port in use
    portName = "";                             // nothing to show
    demoMode = true;                           // fall back to demo data
  }                                            // end of try
}                                              // end of openPort()


void closePort() {                             // closes the current port if one is open
  if (port != null) {                          // is a port open?
    try {                                      // closing can fail if the cable was pulled
      port.stop();                             // close the port
    } catch (Exception e) {                    // ignore errors while closing
      println("Error closing port: " + e.getMessage());  // just report it
    }                                          // end of try
    println("Closed " + portName);             // console message
  }                                            // end of open check
  port = null;                                 // no port in use now
  portName = "";                               // clear the status bar name
  headerRequestAt = 0;                         // cancel any pending header request
}                                              // end of closePort()


void nextPort() {                              // P key: move on to the next candidate port
  scanPorts();                                 // re-scan in case the board was just plugged in
  if (candidates.size() == 0) {                // still nothing found?
    closePort();                               // make sure nothing is left open
    demoMode = true;                           // show demo data instead
    println("No ESP32 port available - demo mode");  // tell the user
    return;                                    // stop here
  }                                            // end of empty check
  int next = (portIndex + 1) % candidates.size();  // next index, wrapping round to 0
  openPort(next);                              // close the current port and open the next
}                                              // end of nextPort()


void toggleDemo() {                            // D key: switch demo mode on or off
  if (demoMode) {                              // currently in demo mode?
    demoMode = false;                          // try to go back to real data
    data = blankData();                        // clear the demo values
    lastRxMillis = millis();                   // grace period before showing STALE
    if (port == null) {                        // no port open yet?
      scanPorts();                             // look for one
      if (candidates.size() > 0) {             // found one?
        openPort(0);                           // open it (falls back to demo if it fails)
      } else {                                 // still nothing
        demoMode = true;                       // stay in demo mode
        println("No port to use - staying in demo mode");  // tell the user
      }                                        // end of candidate check
    }                                          // end of port check
  } else {                                     // currently showing real data
    demoMode = true;                           // switch to demo data
  }                                            // end of mode check
  println("Demo mode: " + demoMode);           // console message
}                                              // end of toggleDemo()


void checkHeaderRequest() {                    // sends the delayed '?' after the port opens
  if (headerRequestAt != 0 && millis() >= headerRequestAt) {  // time to send it?
    headerRequestAt = 0;                       // only send once
    requestHeader();                           // send '?'
  }                                            // end of time check
}                                              // end of checkHeaderRequest()


void requestHeader() {                         // PC -> MCU: ask the ESP32 for its header line
  if (port == null) {                          // no port open?
    println("No port open - cannot send '?'"); // tell the user
    return;                                    // stop here
  }                                            // end of port check
  try {                                        // writing can fail if the cable was pulled
    port.write('?');                           // send one '?' character to the ESP32
    println("Sent '?' - waiting for #EV header");  // console message
  } catch (Exception e) {                      // write failed
    println("Could not send '?': " + e.getMessage());  // report why
  }                                            // end of try
}                                              // end of requestHeader()


void serialEvent(Serial p) {                   // called (on the serial thread) for each full line
  try {                                        // never let a bad line crash the sketch
    String line = p.readStringUntil('\n');     // read up to and including the newline
    if (line == null) {                        // nothing complete yet?
      return;                                  // wait for the next event
    }                                          // end of null check
    line = trim(line);                         // remove \r, \n and spaces at the ends
    if (line.startsWith("#EV")) {              // header line (boot or reply to '?')
      println("Header: " + line);              // print it, but don't parse it as data
      return;                                  // done with this line
    }                                          // end of header check
    if (!line.startsWith("$EV")) {             // boot-ROM garbage or other text
      return;                                  // ignore it
    }                                          // end of prefix check
    String[] tok = split(line, ',');           // split the line at every comma
    if (tok.length != NUM_FIELDS + 1 || !tok[0].equals("$EV")) {  // wrong number of fields?
      return;                                  // ignore half lines
    }                                          // end of length check
    int[] parsed = new int[NUM_FIELDS];        // NEW array for this line
    for (int i = 0; i < NUM_FIELDS; i++) {     // convert each field
      parsed[i] = Integer.parseInt(trim(tok[i + 1]));  // throws if it is not a whole number
    }                                          // end of loop
    if (demoMode) {                            // showing demo data?
      return;                                  // don't mix real and demo values
    }                                          // end of demo check
    data = parsed;                             // swap in the whole array in one step
    lastRxMillis = millis();                   // remember when good data arrived
  } catch (Exception e) {                      // bad number or other problem
    // ignore this line and wait for the next one
  }                                            // end of try
}                                              // end of serialEvent()


boolean isStale() {                            // true when real data has stopped arriving
  return !demoMode && port != null && millis() - lastRxMillis > STALE_MS;  // too long since last line
}                                              // end of isStale()


void keyPressed() {                            // called when a key is pressed
  if (key == 'p' || key == 'P') {              // P = next port
    nextPort();                                // move on to the next port
  } else if (key == 'd' || key == 'D') {       // D = demo mode
    toggleDemo();                              // switch demo on or off
  } else if (key == 'h' || key == 'H') {       // H = header
    requestHeader();                           // send '?' to the ESP32
  }                                            // end of key checks
}                                              // end of keyPressed()


// =====================================================================
//  Demo data
// =====================================================================

void updateDemo() {                            // makes plausible fake data at 10 Hz
  if (millis() - lastDemoMillis < DEMO_PERIOD_MS) {  // less than 100 ms since last update?
    return;                                    // keep the current values
  }                                            // end of timing check
  lastDemoMillis = millis();                   // remember this update time
  float t = millis() / 1000.0;                 // time in seconds
  int[] d = new int[NUM_FIELDS];               // NEW array, same path as real data
  demoSeq = demoSeq + 1;                       // next sequence number
  d[F_SEQ] = demoSeq;                          // store the sequence number
  d[F_IGN] = 1;                                // ignition on
  d[F_GEAR] = int(t / 4) % 5;                  // gear N,1,2,3,4 changing every 4 s
  boolean braking = (t % 8.0) > 6.0;           // brake for the last 2 s of every 8 s
  if (braking) {                               // braking phase
    d[F_THR] = 0;                              // foot off the throttle
    d[F_BRK] = int(60 + 40 * sin(t * 3));      // brake pedal 20..100 %
  } else {                                     // driving phase
    d[F_THR] = int(50 + 50 * sin(t * 1.3));    // throttle sweeps 0..100 %
    d[F_BRK] = 0;                              // brake released
  }                                            // end of braking check
  d[F_STEER] = int(100 * sin(t * 0.6));        // steering swings left and right
  d[F_LDR] = int(2400 + 900 * sin(t * 0.3));   // light level drifts across ~2400
  d[F_HEAD] = (d[F_LDR] > 2400) ? 1 : 0;       // headlights on when it is dark
  d[F_TEMP] = int(330 + 120 * sin(t * 0.2));   // 21..45 C over ~30 s (tenths of a degree)
  d[F_HOT] = (d[F_TEMP] >= 400) ? 1 : 0;       // overheat warning at 40.0 C
  int indPhase = int(t / 6) % 3;               // 0 = off, 1 = left, 2 = right (6 s each)
  d[F_INDL] = (indPhase == 1) ? 1 : 0;         // left indicator switched on
  d[F_INDR] = (indPhase == 2) ? 1 : 0;         // right indicator switched on
  boolean blinkOn = (millis() % 667) < 333;    // ~1.5 Hz flash, half on, half off
  d[F_LAMPL] = (on(d[F_INDL]) && blinkOn) ? 1 : 0;  // left lamp flashes
  d[F_LAMPR] = (on(d[F_INDR]) && blinkOn) ? 1 : 0;  // right lamp flashes
  if (d[F_GEAR] == 0 || braking) {             // neutral or braking?
    d[F_MOTOR] = 0;                            // motor off
  } else {                                     // in gear and not braking
    d[F_MOTOR] = int(d[F_THR] * 2.55);         // motor duty follows the throttle (0..255)
  }                                            // end of motor check
  if (braking) {                               // braking?
    d[F_BRAKE] = int(d[F_BRK] * 2.55);         // brake light brightness follows the pedal
  } else if (on(d[F_HEAD])) {                  // headlights on?
    d[F_BRAKE] = 40;                           // dim tail light
  } else {                                     // daytime, not braking
    d[F_BRAKE] = 0;                            // brake light off
  }                                            // end of brake light check
  boolean gearPress = (t % 4.0) < 0.3;         // first 0.3 s after each gear change
  d[F_S1] = (gearPress && d[F_GEAR] != 0) ? 1 : 0;  // S1 = gear up
  d[F_S3] = (gearPress && d[F_GEAR] == 0) ? 1 : 0;  // S3 = gear down (back to N)
  boolean indPress = (t % 6.0) < 0.3;          // first 0.3 s of each indicator phase
  d[F_S4] = (indPress && indPhase == 1) ? 1 : 0;    // S4 = left indicator
  d[F_S2] = (indPress && indPhase == 2) ? 1 : 0;    // S2 = right indicator
  d[F_JS] = ((t % 20.0) < 0.3) ? 1 : 0;        // joystick button tapped every 20 s
  data = d;                                    // swap in the whole array, like serialEvent()
}                                              // end of updateDemo()


// =====================================================================
//  Drawing
// =====================================================================

void drawPanels() {                            // draws the rounded panel backgrounds
  stroke(cEdge);                               // panel outline colour
  strokeWeight(1);                             // thin outline
  fill(cPanel);                                // panel fill colour
  rect(20, 60, 270, 380, 10);                  // left panel: pedals
  rect(305, 60, 390, 380, 10);                 // centre panel: gauge, gear, indicators
  rect(710, 60, 270, 380, 10);                 // right panel: steering, headlights
  rect(20, 455, 400, 120, 10);                 // bottom left: switches
  rect(435, 455, 130, 120, 10);                // bottom middle: 7-segment
  rect(580, 455, 400, 120, 10);                // bottom right: temperature
  fill(cDim);                                  // label colour
  textAlign(LEFT, CENTER);                     // labels start at their x position
  textSize(12);                                // small label text
  text("PEDALS", 35, 78);                      // left panel title
  text("MOTOR POWER", 320, 78);                // centre panel title
  text("STEERING", 725, 78);                   // right panel title
  text("SWITCHES", 35, 472);                   // switches panel title
  text("TEMPERATURE", 595, 472);               // temperature panel title
  textAlign(CENTER, CENTER);                   // centred title for the small panel
  text("LAMPS", 500, 470);                     // 7-segment panel title
}                                              // end of drawPanels()


void drawTitleBar(int[] d) {                   // heading text and ignition badge
  fill(cText);                                 // main text colour
  textAlign(LEFT, CENTER);                     // left-aligned heading
  textSize(22);                                // large heading
  text("EV CONTROLLER DASHBOARD", 20, 30);     // heading
  boolean ign = on(d[F_IGN]);                  // ignition state
  noStroke();                                  // no outline on the badge
  fill(ign ? cGreen : cOff);                   // green when on, grey when off
  rect(840, 14, 140, 32, 16);                  // pill-shaped badge
  fill(ign ? cBg : cDim);                      // dark text on green, grey text on grey
  textAlign(CENTER, CENTER);                   // centre text in the badge
  textSize(14);                                // badge text size
  text(ign ? "IGNITION ON" : "IGNITION OFF", 910, 30);  // badge text
}                                              // end of drawTitleBar()


void drawPedals(int[] d) {                     // throttle bar, brake bar and brake light
  drawVBar(45, 110, 45, 250, d[F_THR], cGreen, "THROTTLE");  // throttle bar
  drawVBar(115, 110, 45, 250, d[F_BRK], cRed, "BRAKE");      // brake pedal bar
  drawBrakeLight(230, 200, d[F_BRAKE]);        // brake light glow
}                                              // end of drawPedals()


void drawVBar(float x, float y, float w, float h, int pct, color c, String label) {  // one vertical % bar
  pct = constrain(pct, 0, 100);                // keep the value inside 0..100
  float fillH = h * pct / 100.0;               // height of the filled part
  noStroke();                                  // no outlines
  fill(cOff);                                  // empty track colour
  rect(x, y, w, h, 4);                         // empty track
  fill(c);                                     // bar colour
  rect(x, y + h - fillH, w, fillH, 4);         // filled part grows upwards
  fill(cText);                                 // value text colour
  textAlign(CENTER, CENTER);                   // centre under the bar
  textSize(18);                                // value text size
  text(pct + "%", x + w / 2, y + h + 22);      // percentage value
  fill(cDim);                                  // label colour
  textSize(11);                                // label text size
  text(label, x + w / 2, y + h + 46);          // bar name
}                                              // end of drawVBar()


void drawBrakeLight(float cx, float cy, int duty) {  // red glow sized by brake light duty
  duty = constrain(duty, 0, 255);              // keep the duty inside 0..255
  noStroke();                                  // no outlines on the glow
  for (int i = 5; i >= 1; i--) {               // five rings, largest first
    fill(255, 40, 30, duty * 0.12);            // faint red, stronger with more duty
    ellipse(cx, cy, 36 + i * 14, 36 + i * 14); // rings overlap so the centre is brightest
  }                                            // end of loop
  fill(lerpColor(cOff, cRed, duty / 255.0));   // lamp colour from grey (off) to red (full)
  ellipse(cx, cy, 36, 36);                     // the lamp itself
  fill(cDim);                                  // label colour
  textAlign(CENTER, CENTER);                   // centred text
  textSize(11);                                // label text size
  text("BRAKE LIGHT", cx, cy + 70);            // label
  fill(cText);                                 // value colour
  textSize(14);                                // value text size
  text("duty " + duty, cx, cy + 90);           // PWM duty value
}                                              // end of drawBrakeLight()


void drawSpeedo(int[] d) {                     // 270 degree motor power gauge
  float cx = 500;                              // gauge centre x
  float cy = 260;                              // gauge centre y
  float dia = 300;                             // gauge diameter
  int duty = constrain(d[F_MOTOR], 0, 255);    // motor PWM duty 0..255
  float pct = duty * 100.0 / 255.0;            // duty as a percentage
  float startA = radians(135);                 // gauge starts bottom-left
  float endA = radians(405);                   // and ends bottom-right (270 degree sweep)
  noFill();                                    // arcs are outlines only
  strokeCap(SQUARE);                           // flat arc ends
  strokeWeight(18);                            // thick arc
  stroke(cOff);                                // empty track colour
  arc(cx, cy, dia, dia, startA, endA);         // full background track
  if (pct > 0.5) {                             // anything to draw?
    stroke(lerpColor(cGreen, cRed, pct / 100.0));  // green at low power, red at full
    arc(cx, cy, dia, dia, startA, startA + (endA - startA) * pct / 100.0);  // filled part
  }                                            // end of value check
  strokeCap(ROUND);                            // back to the normal line ends
  strokeWeight(2);                             // thin tick marks
  textAlign(CENTER, CENTER);                   // centred tick labels
  textSize(11);                                // tick label size
  for (int i = 0; i <= 10; i++) {              // tick every 10 %
    float a = radians(135 + 27 * i);           // angle for this tick
    stroke(cDim);                              // tick colour
    line(cx + cos(a) * 122, cy + sin(a) * 122, cx + cos(a) * 134, cy + sin(a) * 134);  // tick mark
    if (i % 2 == 0) {                          // label every 20 %
      fill(cDim);                              // label colour
      text(str(i * 10), cx + cos(a) * 108, cy + sin(a) * 108);  // 0, 20, 40 ...
    }                                          // end of label check
  }                                            // end of loop
  fill(cText);                                 // value colour
  textSize(26);                                // value text size
  text(round(pct) + "%", cx, cy + 75);         // motor power percentage
  fill(cDim);                                  // small text colour
  textSize(11);                                // small text size
  text("PWM " + duty + " / 255", cx, cy + 135);  // raw duty in the gap at the bottom
}                                              // end of drawSpeedo()


void drawGear(int[] d) {                       // big gear letter in the middle of the gauge
  int gear = d[F_GEAR];                        // current gear number
  String s = "?";                              // shown if the value is out of range
  if (gear == 0) {                             // neutral?
    s = "N";                                   // show N
  } else if (gear >= 1 && gear <= 4) {         // a forward gear?
    s = str(gear);                             // show the number
  }                                            // end of gear check
  fill(cDim);                                  // label colour
  textAlign(CENTER, CENTER);                   // centred text
  textSize(12);                                // label size
  text("GEAR", 500, 190);                      // label above the letter
  fill(gear == 0 ? cAmber : cText);            // amber for neutral, white for a gear
  textSize(90);                                // very large letter
  text(s, 500, 240);                           // gear letter
}                                              // end of drawGear()


void drawIndicators(int[] d) {                 // left and right indicator arrows
  drawArrow(345, 115, -1, on(d[F_INDL]), on(d[F_LAMPL]));  // left arrow
  drawArrow(655, 115, 1, on(d[F_INDR]), on(d[F_LAMPR]));   // right arrow
}                                              // end of drawIndicators()


void drawArrow(float cx, float cy, int dir, boolean active, boolean lit) {  // one arrow, dir -1 = left, 1 = right
  strokeWeight(2);                             // outline thickness
  if (lit) {                                   // lamp lit right now?
    fill(cGreen);                              // solid green
    stroke(cGreen);                            // green outline
  } else if (active) {                         // indicator on but lamp in its off half
    noFill();                                  // hollow
    stroke(cGreen);                            // green outline
  } else {                                     // indicator off
    noFill();                                  // hollow
    stroke(cOff);                              // grey outline
  }                                            // end of state check
  beginShape();                                // start the arrow outline
  vertex(cx - dir * 22, cy - 7);               // tail top
  vertex(cx, cy - 7);                          // shaft top meets head
  vertex(cx, cy - 18);                         // head top corner
  vertex(cx + dir * 24, cy);                   // arrow tip
  vertex(cx, cy + 18);                         // head bottom corner
  vertex(cx, cy + 7);                          // shaft bottom meets head
  vertex(cx - dir * 22, cy + 7);               // tail bottom
  endShape(CLOSE);                             // join back to the start
}                                              // end of drawArrow()


void drawSteering(int[] d) {                   // steering wheel turned by the steer value
  float cx = 845;                              // wheel centre x
  float cy = 175;                              // wheel centre y
  int steer = constrain(d[F_STEER], -100, 100);  // keep inside -100..100
  float angle = map(steer, -100, 100, -90, 90);  // -90 (full left) .. +90 (full right) degrees
  pushMatrix();                                // save the current drawing position
  translate(cx, cy);                           // move the origin to the wheel centre
  rotate(radians(angle));                      // turn the wheel (negative = anticlockwise = left)
  noFill();                                    // rim is an outline
  stroke(cText);                               // rim colour
  strokeWeight(10);                            // thick rim
  ellipse(0, 0, 140, 140);                     // rim
  strokeWeight(8);                             // spoke thickness
  line(-14, 0, -64, 0);                        // left spoke
  line(14, 0, 64, 0);                          // right spoke
  line(0, 14, 0, 64);                          // bottom spoke
  noStroke();                                  // solid shapes next
  fill(cText);                                 // hub colour
  ellipse(0, 0, 30, 30);                       // hub
  fill(cRed);                                  // top-centre marker colour
  rect(-5, -76, 10, 12);                       // marker shows which way is straight ahead
  popMatrix();                                 // back to the normal drawing position
  String txt = "CENTRE";                       // text for zero steering
  if (steer < 0) {                             // turning left?
    txt = "LEFT " + (-steer);                  // show left amount
  } else if (steer > 0) {                      // turning right?
    txt = "RIGHT " + steer;                    // show right amount
  }                                            // end of direction check
  fill(cText);                                 // value colour
  textAlign(CENTER, CENTER);                   // centred text
  textSize(16);                                // value size
  text(txt, cx, cy + 100);                     // steering value under the wheel
}                                              // end of drawSteering()


void drawHeadlights(int[] d) {                 // headlight icon and LDR bar
  boolean head = on(d[F_HEAD]);                // headlights on?
  color c = head ? cBlue : cOff;               // blue when on, grey when off
  float x = 760;                               // icon centre x
  float y = 335;                               // icon centre y
  fill(cDim);                                  // label colour
  textAlign(LEFT, CENTER);                     // left-aligned label
  textSize(12);                                // label size
  text("HEADLIGHTS", 725, 300);                // section label
  noStroke();                                  // solid lamp body
  fill(c);                                     // lamp colour
  arc(x, y, 40, 40, HALF_PI, PI + HALF_PI, CHORD);  // D-shaped lamp body, flat side right
  stroke(c);                                   // beam colour
  strokeWeight(3);                             // beam thickness
  for (int i = -1; i <= 1; i++) {              // three beams
    line(x + 6, y + i * 10, x + 30, y + i * 10 + 4);  // beam angled slightly down
  }                                            // end of loop
  fill(head ? cBlue : cDim);                   // state text colour
  textSize(20);                                // state text size
  text(head ? "ON" : "OFF", x + 50, y);        // ON / OFF text
  int ldr = constrain(d[F_LDR], 0, 4095);      // raw LDR reading
  fill(cDim);                                  // label colour
  textSize(12);                                // label size
  text("LDR  " + ldr, 725, 375);               // raw value
  noStroke();                                  // bar has no outline
  fill(cOff);                                  // empty track
  rect(725, 388, 240, 14, 7);                  // LDR track
  fill(cAmber);                                // bar colour
  rect(725, 388, 240 * ldr / 4095.0, 14, 7);   // filled part (longer = darker)
  fill(cDim);                                  // scale label colour
  textSize(10);                                // scale label size
  textAlign(LEFT, CENTER);                     // left end label
  text("light", 725, 415);                     // left end of the scale
  textAlign(RIGHT, CENTER);                    // right end label
  text("dark", 965, 415);                      // right end of the scale
}                                              // end of drawHeadlights()


void drawSwitches(int[] d) {                   // lamps for the four switches and joystick button
  String[] names = {"S1", "S2", "S3", "S4", "JS"};  // switch names
  String[] jobs = {"GEAR +", "IND R", "GEAR -", "IND L", "START"};  // what each one does
  int[] idx = {F_S1, F_S2, F_S3, F_S4, F_JS};  // matching transmit array fields
  for (int i = 0; i < 5; i++) {                // draw each switch
    float x = 70 + i * 75;                     // lamp centre x
    float y = 512;                             // lamp centre y
    boolean pressed = on(d[idx[i]]);           // is it pressed now?
    strokeWeight(2);                           // ring thickness
    stroke(pressed ? cGreen : cEdge);          // green ring when pressed
    fill(pressed ? cGreen : cOff);             // lit when pressed
    ellipse(x, y, 34, 34);                     // the lamp
    fill(cText);                               // name colour
    textAlign(CENTER, CENTER);                 // centred text
    textSize(13);                              // name size
    text(names[i], x, y + 30);                 // switch name
    fill(cDim);                                // job colour
    textSize(10);                              // job size
    text(jobs[i], x, y + 47);                  // what the switch does
  }                                            // end of loop
}                                              // end of drawSwitches()


void drawSevenSeg(int[] d) {                   // 7-segment digit mirroring the physical lamps
  float xL = 475;                              // left edge x
  float xR = 525;                              // right edge x
  float yT = 485;                              // top segment y
  float yM = 522;                              // middle segment y
  float yB = 559;                              // bottom segment y
  float t = 5;                                 // half thickness of a segment
  float gap = 2;                               // gap between segments
  color cLeft = on(d[F_LAMPL]) ? cAmber : cOff;   // e + f follow the left lamp
  color cRight = on(d[F_LAMPR]) ? cAmber : cOff;  // b + c follow the right lamp
  noStroke();                                  // segments are solid
  fill(on(d[F_HEAD]) ? cBlue : cOff);          // a = headlight
  segH(xL + t + gap, xR - t - gap, yT, t);     // segment a (top)
  fill(cRight);                                // right lamp colour
  segV(xR, yT + t + gap, yM - t - gap, t);     // segment b (top right)
  segV(xR, yM + t + gap, yB - t - gap, t);     // segment c (bottom right)
  fill(cOff);                                  // d drawn unlit first
  segH(xL + t + gap, xR - t - gap, yB, t);     // segment d background
  fill(255, 40, 30, constrain(d[F_BRAKE], 0, 255));  // brake light red, alpha = brake duty
  segH(xL + t + gap, xR - t - gap, yB, t);     // segment d (bottom) = brake light
  fill(cLeft);                                 // left lamp colour
  segV(xL, yM + t + gap, yB - t - gap, t);     // segment e (bottom left)
  segV(xL, yT + t + gap, yM - t - gap, t);     // segment f (top left)
  fill(cOff);                                  // g is always off
  segH(xL + t + gap, xR - t - gap, yM, t);     // segment g (middle)
}                                              // end of drawSevenSeg()


void segH(float x1, float x2, float y, float t) {  // horizontal segment from x1 to x2 at height y
  quad(x1, y, x1 + t, y - t, x2 - t, y - t, x2, y);  // upper half (pointed ends)
  quad(x1, y, x1 + t, y + t, x2 - t, y + t, x2, y);  // lower half
}                                              // end of segH()


void segV(float x, float y1, float y2, float t) {  // vertical segment from y1 to y2 at x
  quad(x, y1, x + t, y1 + t, x + t, y2 - t, x, y2);  // right half (pointed ends)
  quad(x, y1, x - t, y1 + t, x - t, y2 - t, x, y2);  // left half
}                                              // end of segV()


void drawTemperature(int[] d) {                // temperature read-out
  int raw = d[F_TEMP];                         // temperature in tenths of a degree
  boolean hot = on(d[F_HOT]);                  // over-temperature flag
  textAlign(CENTER, CENTER);                   // centred text
  if (raw == TEMP_NOT_FITTED) {                // no sensor fitted?
    fill(cDim);                                // grey text
    textSize(44);                              // big text
    text("--", 780, 512);                      // no reading
    textSize(11);                              // small text
    text("sensor not fitted", 780, 552);       // explain the dashes
  } else {                                     // a real reading
    float tC = raw / 10.0;                     // convert tenths to degrees
    fill(hot ? cRed : cText);                  // red when hot
    textSize(44);                              // big text
    text(nf(tC, 0, 1) + " \u00B0C", 780, 512); // e.g. 23.4 degrees C (\u00B0 = degree sign)
    if (hot) {                                 // over temperature?
      noStroke();                              // no outline on the badge
      fill(cRed);                              // red badge
      rect(895, 497, 70, 30, 8);               // HOT badge
      fill(cText);                             // badge text colour
      textSize(14);                            // badge text size
      text("HOT!", 930, 512);                  // warning text
    }                                          // end of hot check
  }                                            // end of sensor check
}                                              // end of drawTemperature()


void drawStatusBar(int[] d) {                  // bottom bar: port state, seq, key help
  noStroke();                                  // no outline
  fill(cPanel);                                // bar colour
  rect(0, 588, width, 32);                     // full-width bar along the bottom
  textAlign(LEFT, CENTER);                     // left-aligned status text
  textSize(13);                                // status text size
  String s;                                    // status text
  if (demoMode) {                              // demo data on screen?
    fill(cAmber);                              // amber
    s = "DEMO MODE";                           // say so
  } else if (port == null) {                   // no port open?
    fill(cRed);                                // red
    s = "NO PORT";                             // say so
  } else {                                     // a real port is open
    fill(cGreen);                              // green
    s = "PORT " + portName + " @ " + BAUD;     // port name and baud rate
  }                                            // end of state check
  text(s, 15, 604);                            // draw the status text
  if (isStale()) {                             // no data for over 1.5 s?
    fill(cRed);                                // red
    text("  STALE", 15 + textWidth(s), 604);   // warning straight after the port name
  }                                            // end of stale check
  fill(cText);                                 // seq colour
  textAlign(CENTER, CENTER);                   // centred seq number
  text("SEQ " + d[F_SEQ], 500, 604);           // sequence number of the last line
  fill(cDim);                                  // help text colour
  textAlign(RIGHT, CENTER);                    // right-aligned help text
  text("P: next port   D: demo   H: header", 985, 604);  // key help
}                                              // end of drawStatusBar()

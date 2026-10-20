/*
  EV Controller Interface - ESP32 port of my MSP430 (Energia) firmware.cpp

  Board:    ESP32-WROVER-E DevKit (DevKitC-VE)
  Build:    PlatformIO in VS Code or Cursor (see README.md in this folder)

  The logic is the same as the MSP430 version. Each change for the ESP32 is
  marked "ESP32:" with the reason, and each pin #define notes the MSP430 pin it
  replaces. Everything else is unchanged.
  Ideas for improving the logic are in SUGGESTED_IMPROVEMENTS.md (not applied).
*/

#include <Arduino.h>  // ESP32: PlatformIO needs this in a .cpp file (Energia added it for you)

#define RIL 21  //define GPIO21 as right indicator light (MSP430: P2.0) 

#define LIL 19  //define GPIO19 as left indicator light (MSP430: P2.1)  

#define S1 25 //define GPIO25 as switch 1 (MSP430: P2.2)  

#define S3 26 //define GPIO26 as switch 3 (MSP430: P2.3)  

#define S4 27 //define GPIO27 as switch 4 (MSP430: P2.4) 

#define S2 13 //define GPIO13 as switch 2 (MSP430: P2.5) 

#define Motor 23  //define GPIO23 as the motor (MSP430: P1.6) 

#define BL 18 //define GPIO18 as the brakelight (MSP430: P2.6)  

#define HL 22 //define GPIO22 as headlight (MSP430: P2.7) 

// ESP32: on the ESP32 the names A0, A3, A4... point at different pins than on the
// MSP430, so the analog inputs get their own names. All are on ADC1 (GPIO32-39).

#define JS_CENTRE_PIN 33  //joystick centre button (MSP430: A3 / P1.3)

#define JS_U_D_PIN 34  //joystick up and down (MSP430: A4 / P1.4)

#define JS_L_R_PIN 39  //joystick left and right (MSP430: A5 / P1.5)

#define LIGHT_PIN 36  //light sensor LDR (MSP430: A0 / P1.0)

#define TEMP_PIN 35  //temperature sensor LM35 (MSP430: A7 / P1.7)

// ESP32: the MSP430 ADC gives 0-1023 for 0-3.3 V. The ESP32 ADC is 12 bit and not
// straight-line, so read the calibrated millivolts and scale them to the same
// 0-1023 range. That keeps every number below (550, 500, 220, 0.322) the same.

int analogRead10(int pin) {

  if (analogRead(pin) == 0) {  // below about 0.1 V: the ESP32 can't measure this low,

    return 0;                  // so report 0 like the MSP430 did

  }

  long counts = analogReadMilliVolts(pin) * 1023L / 3300;  // mV -> 0..1023 like the MSP430

  if (counts > 1023) {  // never above the MSP430's maximum

    counts = 1023;

  }

  return (int)counts;  // the reading on the MSP430's 0-1023 scale

}

int gearPos = 0;  //integer for gear position  

int displayValue[4] = {1, 6, 8, 48}; //array for indicators, and light on the 7seg  

void setup() {  

  Serial.begin(9600); // set baud rate  

  // analogReference(DEFAULT); // ESP32: not available (and not needed): the ESP32 reads 0-3.3 V already 

  pinMode(RIL, OUTPUT); //set pins to output  

  pinMode(LIL, OUTPUT); //set pins to output  

  pinMode(HL, OUTPUT); //set pins to output  

  pinMode(BL, OUTPUT); //set pins to output  

  pinMode(Motor, OUTPUT);  // set P1.6 to output  

  // ESP32: the MSP430 for loop used Energia pin numbers 10-13 (P2.2-P2.5). On the ESP32
  // those numbers are different pins (GPIO10-11 are wired to the flash chip), so each
  // switch pin is set on its own instead.

  pinMode(S1, INPUT_PULLUP);  //set switch pins to input pullup 

  pinMode(S2, INPUT_PULLUP);  //set switch pins to input pullup 

  pinMode(S3, INPUT_PULLUP);  //set switch pins to input pullup 

  pinMode(S4, INPUT_PULLUP);  //set switch pins to input pullup 

}  

void loop() {  

  int JS_Centre = analogRead10(JS_CENTRE_PIN); // assign the input from the joystick centre button 

  int JS_U_D = analogRead10(JS_U_D_PIN); // assign the up and down inputs from the joystick 

  int JS_L_R = analogRead10(JS_L_R_PIN); // assign the left and right inputs from the joystick  

  int lightSensor = analogRead10(LIGHT_PIN); // assign the lightsensor value 

  int tempSensor = analogRead10(TEMP_PIN); // assign the temperature value   

  //Serial.print("JS_Centre");  //print the value to the monitor  

  //Serial.print("\t");  //print the value to the monitor  

  //Serial.print("JS_U_D");  //print the value to the monitor  

  // Serial.print("\t");  //print the value to the monitor  

  // Serial.print("JS_L_R");//print the value to the monitor  

  // Serial.print("\t");  //print the value to the monitor  

  // Serial.print("lightSensor");//print the value to the monitor  

  // Serial.print("\t");  //print the value to the monitor  

  Serial.print("Gear Position"); 

  Serial.print("\t");  //print the value to the monitor  

  float Temp = tempSensor * 0.322;  //convert the pwm value from the tempsensor to celsius 

  Serial.println("Temp Celsius");//print the value to the monitor  

  //  Serial.print(JS_Centre);  //print the value to the monitor  

  //  Serial.print("\t");  //print the value to the monitor  

  //  Serial.print(JS_U_D);  //print the value to the monitor  

  // Serial.print("\t");  //print the value to the monitor  

  //  Serial.print(JS_L_R);//print the value to the monitor  

  //  Serial.print("\t");  //print the value to the monitor  

  //  Serial.print(lightSensor);  //print the value to the monitor  

  Serial.print(gearPos); 

  Serial.print("\t");  //print the value to the monitor  

  Serial.println(Temp);  //print the value to the monitor  

  if (JS_U_D > 550) { //control the motor using the inputs from the joystick 

    analogWrite(Motor, JS_U_D );  

  } else {  

    analogWrite(Motor, 0);  

  }  

  if (JS_U_D < 500) {  

    analogWrite(BL, JS_U_D);  

  } else {  

    analogWrite(BL, 0);  

  }  

  if (lightSensor > 220) {  //controlling the headlight using the lightsensor 

    digitalWrite(HL, HIGH);  

  } else {  

    digitalWrite(HL, LOW);  

  }  

  if (digitalRead(S2) == LOW) { //controlling the right indicator using switch S2 

    delay(250);   //debounce 

    digitalWrite(RIL, HIGH);  

  } else {  

    digitalWrite(RIL, LOW); 

  }  

  if (digitalRead(S4) == LOW) { //controlling the left indicator using switch S4  

    delay(250);  

    digitalWrite(LIL, HIGH); 

  } else {  

    digitalWrite(LIL, LOW); 

  }  

  if (digitalRead(S1) == LOW) { //change gear position up using S1 

    delay(250); //debounce delay  

    gearPos++;//increment  

  if (gearPos>4){ 

    gearPos = 0; 

  } 

  }  

  if (digitalRead(S3) == LOW) { //change gear position down using S3  

    delay(250); //debounce delay  

    gearPos--;//increment  

  if (gearPos<0){ 

    gearPos = 0; 

  } 

  }  

}  
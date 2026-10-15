#define RIL P2_0  //define P2.0 as right indicator light 

#define LIL P2_1  //define P2.1 as left indicator light  

#define S1 P2_2 //define P2.2 as switch 1  

#define S3 P2_3 //define P2.3 as switch 3  

#define S4 P2_4 //define P2.4 as switch 4 

#define S2 P2_5 //define P2.5 as switch 2 

#define Motor P1_6  //define P1.6 as the motor 

#define BL P2_6 //define P2.6 as the brakelight  

#define HL P2_7 //define P2.7 as headlight 

int gearPos = 0;  //integer for gear position  

int displayValue[4] = {1, 6, 8, 48}; //array for indicators, and light on the 7seg  

void setup() {  

  Serial.begin(9600); // set baud rate  

  analogReference(DEFAULT); // sets Vref to default of 3.3V  

  pinMode(RIL, OUTPUT); //set pins to output  

  pinMode(LIL, OUTPUT); //set pins to output  

  pinMode(HL, OUTPUT); //set pins to output  

  pinMode(BL, OUTPUT); //set pins to output  

  pinMode(Motor, OUTPUT);  // set P1.6 to output  

  for (int pin = 10; pin <= 13; pin++) {    //using a for loop to set the pins to input pullup 

    pinMode(pin, INPUT_PULLUP);  

  }  

}  

void loop() {  

  int JS_Centre = analogRead(A3); // assign the input from the joystick centre button 

  int JS_U_D = analogRead(A4); // assign the up and down inputs from the joystick 

  int JS_L_R = analogRead(A5); // assign the left and right inputs from the joystick  

  int lightSensor = analogRead(A0); // assign the lightsensor value 

  int tempSensor = analogRead(A7); // assign the temperature value   

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
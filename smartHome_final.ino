/**************************************************************************
  Leonardo Apollar Gama and Rui Wang
  Smart Home System - ECE 2804

  This is the code for the Smart Home Project
  design and implmented as following the requirements for ECE 2804
  and the help and coaching of Professor Peter Han

  This project delivers an DIY solution of an smart home system
  being able to control appliances, monitor weather and perfom automation.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// Humidity and temperatuere sensor declaration - Weather Station
int input = 9; // RC circuit with HS1101 sesnor connected to digital pin D9
int high_time; // initial the high and low time, float period, humidity, and frequency since they are variables.
int low_time;
float period;
int humidity;
String myHumidity;
float frequency;

int ThermistorPin = 0; //int Vo, and the pin0 which was connected to temperature sensor
int Vo;
float R1 = 10000;
float logR2, R2;
int T;
String myT;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

String weather;

// Automated night Light declaration
const int Led = 7;
const int Ldr = A1;

//speaker declaration
const int pin = 3; // read output pin 3

//AC declaration
const int AC = 10; //the led is connected to outpin 10 on arduino

// LED representing Appliances declaration
char character;
int redLed = 13; //define pin for Red LED
int greenLed = 12; //define pin for Green LED
String redVoice = "off"; //string used to determine state of redLED
String greenVoice = "off"; //string used to getermine state of greenLED

// Push Button Declaration
#define button 8
int lastState = LOW;
int currentState;
int doorbell = 1;

//Ultrasonic sensor declaration (HC-SR04)
int trigPin = 4;    // Trigger
int echoPin = 5;    // Echo
long duration, cm, inches;
int intruder_enable = 1;
int intruder = 2;

void setup() {
  // Begin virtual wires, serial at BAUDRATE 9600 ans set serial timeout
  Wire.begin();
  Serial.begin(9600);
  Serial.setTimeout(50);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);// Address 0x3D for 128x64

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();

  // Set humidity sensot input pinmode
  pinMode(input, INPUT);

  // Set photoresistor, led and ldr pinmode
  pinMode(Led, OUTPUT);
  pinMode(Ldr, INPUT);

  //Appliance pin mode
  pinMode(redLed, OUTPUT); //set pin as an output pin
  pinMode(greenLed, OUTPUT); //set pin as an output pin

  // Doorbell pin mode
  pinMode(button, INPUT_PULLUP);

  //Define inputs and outputs for HC-SR04
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //AC=10 as output
  pinMode(AC, OUTPUT);
}

void loop() {
  //Set serial conection to read if there is information being received
  while (Serial.available() > 0) {
    character = Serial.read();       // set character to serial data receveid
  }

  //  Call all smart home system functions.
  WeatherStation();
  NightLight();
  ApplianceControl();
  WeatherApp();
  DoorBell();
  IntruderAlert();
  AirConditioner();
  speaker();

  delay(50);

  DispLCD();
}

void WeatherStation() {
  Vo = analogRead(ThermistorPin);  //read the temperature value
  R2 = R1 * (1023.0 / (float)Vo - 1.0); //calculation for temperature (steinhart-hart formula: T=1/(A+Bln(R)+C(lnR)^3)
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)); //coefficient A,B,C in the steinhart formula
  T = T - 273.15;   // convert to Celsius

  high_time = pulseIn(input, HIGH);
  low_time = pulseIn(input, LOW);
  period = high_time + low_time; //calculate the period
  period = period / 1000;
  frequency = 1000 / period; //calcaute the frequency

//Humidity = RH(%) + 10 * [ (frequency - Fout(max) / (Fout(min) - Fout(max))], RH is the rate of humidity, Fout(max) means the maximum value of frequency in the interval, and Fmin means the minimum
  if ((frequency <= 6186) && (frequency >= 6033)) { //calculation for humidity by frequency, it can be found in HS1101 datasheet
    humidity = 90 + 10 * ((frequency - 6186) / (6033 - 6186));
  }
  else if ((frequency <= 6330) && (frequency > 6186)) {
    humidity = 80 + 10 * ((frequency - 6330) / (6186 - 6330));
  }
  else if ((frequency <= 6468) && (frequency > 6330)) {
    humidity = 70 + 10 * ((frequency - 6468) / (6300 - 6468));
  }
  else if ((frequency <= 6600) && (frequency > 6468)) {
    humidity = 60 + 10 * ((frequency - 6600) / (6468 - 6600));
  }
  else if ((frequency <= 6728) && (frequency > 6600)) {
    humidity = 50 + 10 * ((frequency - 6728) / (6600 - 6728));
  }
  else if ((frequency <= 6853) && (frequency > 6728)) {
    humidity = 40 + 10 * ((frequency - 6853) / (6728 - 6853));
  }
  else if ((frequency <= 6976) && (frequency > 6853)) {
    humidity = 30 + 10 * ((frequency - 6976) / (6853 - 6976));
  }
  else if ((frequency <= 7100) && (frequency > 6976)) {
    humidity = 20 + 10 * ((frequency - 7100) / (6976 - 7100));
  }
  else if ((frequency <= 7224) && (frequency > 7100)) {
    humidity = 10 + 10 * ((frequency - 7224) / (7100 - 7224));
  }
  else if ((frequency <= 7351) && (frequency > 7224)) {
    humidity = 10 * ((frequency - 7351) / (7224 - 7351));
  }
}

void NightLight() {
  int ldrstatus = analogRead(Ldr);

  bool ledstatus = true; //initilize that ledstatus = true
  if (ledstatus == true) { //when ledstatus = true
    if (ldrstatus <= 300) { // darkness > 80%
      digitalWrite(Led, HIGH);  //turn on the led
      bool ledstatus = false; //ledstatus = false
    }
  }
  else if (ledstatus == false) { // when ledstatus = false

    if (ldrstatus >= 600) { //if the darkness < 40
      digitalWrite(Led, LOW); //turn off the led
      bool ledstatus = true; //ledstatus = true
    }
  }
}

// Function responsible for the communication and intepretation of data to be sent and receveid through arduino bluetooth module
// and MIT app inventor app.
void ApplianceControl() {

  //Turns Red LED on or off if data received is "1"
  if (character == 'R') {
    //if state currently off, sets led to HIGH ==> turns led on
    if (redVoice == "off") {
      digitalWrite(redLed, HIGH);
      redVoice = "on";
      character = 'N';
    }
    //if state currently on, sets led to LOW ==> turns led off
    else if (redVoice == "on") {
      digitalWrite(redLed, LOW);
      redVoice =  "off";
      character = 'N';
    }
  }

  //Turns Green LED on or off if data received is "2"
  if (character == 'G') {
    //if state currently off, sets led to HIGH ==> turns led on
    if (greenVoice == "off") {
      digitalWrite(greenLed, HIGH);
      greenVoice = "on";
      character = 'N';
    }
    //if state currently on, sets led to LOW ==> turns led off
    else if (greenVoice == "on") {
      digitalWrite(greenLed, LOW);
      greenVoice = "off";
      character = 'N';
    }
  }
}

void WeatherApp() {
  // Weather Station Send Information to android app
  if (character == 'W') {
    myT = String(T); //convert temperature to string and store in myT
    myT = myT + "˚C"; // add units to temperature
    myHumidity = String(humidity); //convert humidity to string and store in myHumidity
    myHumidity = myHumidity + "%"; // add units to humidity
    weather = myT + "  " + myHumidity; //cocatenate
    Serial.println(weather); // sends back weather to app through serial
    character = 'N';
  }
}

void DoorBell() {
  // Dorbell send notification when button presed and released to android app (PUSH BUTTON DEBOUCEND)
  currentState = digitalRead(button);

  // If push button is pressed and released, send doorbell = 1 thats is equivalent to ASCII 49
  if (lastState == HIGH && currentState == LOW) {
    Serial.println(doorbell);
    character = 'N';
  }

  lastState = currentState;
}

void DispLCD() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("TEMP(C):" + String(T));
  //display.println(T); //display temperature
  display.println("HUM(%):" + String(humidity));
  display.display();
}

void IntruderAlert() {
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  // Convert the time into a distance
  cm = (duration / 2) / 29.1;   // Divide by 29.1 or multiply by 0.0343
  inches = (duration / 2) / 74; // Divide by 74 or multiply by 0.0135

  // If data received is character 'E' enable ultrasonic sensor to read and inpret if there is intruder
  if (character == 'E') {
    intruder_enable = 0;
    character = 'N';
  }
  // If data received is character 'D' disable ultrasonic sensor to read and inpret if there is intruder
  else if (character == 'D') {
    intruder_enable = 1;
    character = 'N';
  }

  // If enabled, and if intruders is detected within range, send intruder = 2 thats is equivalent to ASCII 50
  if (intruder_enable == 0) {
    if (inches <= 10 && inches >= 1) {
      Serial.println(intruder);
      delay(500);
      character = 'N';
    }
  }
}

void AirConditioner() {
  bool tempstatus = true; //intilize temstatus = true
  if (tempstatus == true) { // if  temstatus = true
    if (T > 35) {
      digitalWrite(AC, HIGH);  //when Temperature is higher than 35 C, turns on the led
      bool tempstatus = false; //change to tempstatus = false state
    }
  }

  else if (tempstatus == false) { //when tempstatus = false
    if (T < 33) { //if temperature is less than 33
      digitalWrite(AC, LOW); //turn off the light
      bool tempstatus = true; //change to tempstatus = true status
    }
  }
}

void speaker() {
  if (humidity > 60 && T > 30) { //the alert turns on when Temperature > 30 C and humidity > 60%
    tone(3, 1136, 2000); //a tone(Pin, frequency, duration) 50ms duration.
  }
  else if (humidity > 60) {
    noTone(3);//b //notone is to turn off the previous sound.
    tone(3, 1915, 2000); //c
  }
  else if (T > 30 ) {
    noTone(3);//b
    tone(3, 1700, 2000); //c
  }
  else if (T <= 30 && humidity <= 60) {
    noTone(3); //if no value is reaching the threshold, turn off the speaker.
  }
}

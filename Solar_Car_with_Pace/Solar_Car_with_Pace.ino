// Last Revised 1/2/2018
// Written by Joe Meyer, Fabricator III at the Science Museum of Minnesota in St. Paul
// For Gateway To Science, Bismark, ND

#include <Adafruit_NeoPixel.h>
#include <Wire.h>

#define LEDRingPIN 7
#define HallLinePIN 8
#define HallBeforeLinePIN 9
#define MotorPIN 3
#define LightRelayPIN 4
#define StartButton 14
#define StartBtnLED 15
#define RaceTimePot 16

#define BRIGHTNESS 100
#define NUM_LEDS 48

int lapsToWin = 1; // The number of laps to be completed for win.
long timeToRace = 20000;   // in millisec, time you have to complete the designated # of laps.

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LEDRingPIN, NEO_RGB + NEO_KHZ800);

int resetting = 1, racing = 0, hasWon = 1, ledState=0; // flags
int lapCounter, currentSpeed=0, targetSpeed; 
int timingIndexLED;
unsigned long startMillis=0, previousTimingMillis, previousBlinkMillis, previousRampMillis, currentMillis=0;
long timeElapsed = 0;
int hallLineState=0, hallBeforeState, hallBeforePrevState, hallLinePrevState;   // variables for reading and storing the hall sensors status


void setup() {
  //neo pixels 
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  allLEDS(0,0,255); //all leds to blue, show life!
  
  pinMode(HallLinePIN, INPUT);    
  pinMode(HallBeforeLinePIN, INPUT);    
  pinMode(StartButton, INPUT);  
  pinMode(LightRelayPIN, OUTPUT);   
  pinMode(StartBtnLED, OUTPUT);   
  pinMode(MotorPIN, OUTPUT);
  pinMode(RaceTimePot, INPUT);
    
  Serial.begin(9600);
  analogWrite(MotorPIN,0);
  delay(1000);
  allLEDS(0,0,0); // turn leds off
}

void loop() {
  currentMillis = millis();
  timeElapsed = currentMillis - startMillis;

  // blink lights during reset according to win or lose.
  if (resetting){
    if (hasWon){      // create rainbow pattern. ledState used to change color wheel.
      for(int i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel((i+ledState) & 255));         
      }
      strip.show();
      ledState++;
          
    }else{  // lose lights, blinking red, ledState toggles off/on
      if (currentMillis - previousBlinkMillis >= 500){
        previousBlinkMillis = currentMillis;
        if (ledState ==0){
          allLEDS(255,0,0);
          ledState =1;
        }else{
          allLEDS(0,0,0);
          ledState =0;
        }
      }
    }  
  }

  //store last sensor data
  hallBeforePrevState = hallBeforeState;
  hallLinePrevState = hallLineState;
  //read new sensor data
  hallBeforeState = digitalRead(HallBeforeLinePIN);
  hallLineState = digitalRead(HallLinePIN);  

  // edge detection  
  if (hallBeforeState && !hallBeforePrevState) {  // passed sensor hallBefore
    currentSpeed = 40;
    targetSpeed =40;
    Serial.println("pass Before");
  }
  if (!hallBeforeState && hallBeforePrevState) {  // on sensor hallBefore
    currentSpeed=40;
    targetSpeed =40;    
    Serial.println("ON Before");
  }
  if (hallLineState && !hallLinePrevState) {  // passed sensor hallLine
    targetSpeed = 255;    
    Serial.println("pass LINE");
  }
  if (!hallLineState && hallLinePrevState) {  // On sensor hallLine    
    Serial.println("ON LINE");
    currentSpeed =0;
    targetSpeed =0;
    if (racing==1){
      lapCounter++;
    }
  }

  if (currentMillis - previousRampMillis >= 5){ // every 5 ms check if speed should increase
    previousRampMillis = currentMillis; 
    if (currentSpeed < targetSpeed){ // increase speed by 1 if below target.
      currentSpeed = currentSpeed +1; 
    }
  }
  
  if (!hallLineState && !hallLinePrevState){ // still on sensor hallLine
    if (resetting && timeElapsed>4000){ 
      Serial.println("Done Resetting");
      resetting =0;
      allLEDS(0,0,0);
      strip.setPixelColor(8, 139,0,139); // set color purple to create virtual pace car
      strip.setPixelColor(9, 139,0,139); // 
      strip.setPixelColor(10, 139,0,139); // 
      strip.setPixelColor(11, 139,0,139); //  Start line is at LED 11.
      strip.show();
    }
  }
  
  if (resetting == 1 && timeElapsed > 2000){ // 2 sec delay after Solar lamp is off, then kick in motor reset.
    analogWrite(MotorPIN, currentSpeed);
  }else{
    analogWrite(MotorPIN,0);
    currentSpeed=0;
  }

  if (resetting == 0 && racing ==0){ // ready to race, waiting button press
    
    waitToStart();
  }

  if (racing == 1){    
    // change LED timing ring  
    if (currentMillis - previousTimingMillis >= timeToRace / (strip.numPixels() * lapsToWin)){
      previousTimingMillis = currentMillis;

      strip.setPixelColor(timingIndexLED, 139,0,139); // turn front LED on.
      int timingIndexLEDoff = timingIndexLED -4;
      if (timingIndexLEDoff < 0){
        timingIndexLEDoff = timingIndexLEDoff + 48;
      }
      strip.setPixelColor(timingIndexLEDoff, 0,0,0); // turn back LED off.
      strip.show();
      timingIndexLED++;
      if (timingIndexLED ==48){
        timingIndexLED=0;
      }      
    }
    
    if (lapCounter==lapsToWin){
      Serial.write(timeElapsed); 
      endRace(1);      
    }
    
    if (timeElapsed >timeToRace){
      endRace(0);
    }    
  }
}


void waitToStart(){
  Serial.println("Waiting for button press");
  digitalWrite(StartBtnLED, HIGH);  //light Start button telling visitor they can play!
  while (digitalRead(StartButton) == HIGH) { 
    // wait for user to press start button.
  }  
  racing =1;
  timingIndexLED = 11; // LED 11 is starting line.
  resetting=0;
  timeElapsed =0;  
  currentSpeed=0;
  //read potentiometer
  timeToRace = analogRead(RaceTimePot);
  //convert to milliseconds for race time (10 sec thru 61.15 sec)
  timeToRace = timeToRace*50 + 10000;
  
  digitalWrite(StartBtnLED, LOW);  
  colorWipe(40,255,0,0); //red
  colorWipe(40,255,255,0);  //yellow
  colorWipe(40,0,255,0);  //green
  for(uint16_t i=12; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 0,0,0); // turn off starting light LEDS     
  }
  strip.show(); 
  digitalWrite(LightRelayPIN, HIGH); // turn on the lamp.
  previousTimingMillis = startMillis = millis();   //record start time, initialize perviousTimingMillis for new race.
  lapCounter=0;
}

void endRace(int hw){
  hasWon = hw;
  digitalWrite(LightRelayPIN, LOW);
  racing = 0;
  resetting =1;
  ledState = 0;
  startMillis = millis(); 
}

void colorWipe(int wait, int R, int G, int B){
  for(uint16_t i=0; i<15; i++) {
    strip.setPixelColor(26-i, R,G,B); //color wipe
    delay(wait);
    strip.show();    
  }
}

void allLEDS(int R, int G, int B){   //turn off all LEDs
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, R,G,B);
  }
  strip.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3,0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3,0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0,0);
}

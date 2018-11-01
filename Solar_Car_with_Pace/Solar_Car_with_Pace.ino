#include "stepperControlAdafruit.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>

#define LEDRingPIN 6
#define HallLinePIN 5
#define HallBeforeLinePIN 7
#define LightRelayPIN 4
#define StartButton 13
#define StartBtnLED 11

#define BRIGHTNESS 100
#define NUM_LEDS 48

int lapsToWin = 2; // The number of laps to be completed for win.
const int timeToRace = 30000;   // in millisec, time you have to complete the designated # of laps.

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LEDRingPIN, NEO_RGB + NEO_KHZ800);

//steps per rev, which port, and max RPM
stepper motor(100,2,30);

int resetting = 1, racing = 0, hasWon = 1, ledState=0; // flags
int lapCounter; 
int timingIndexLED;
unsigned long startMillis=0, previousTimingMillis, previousBlinkMillis, currentMillis=0;
long timeElapsed = 0;
int hallLineState=1, hallBeforeState, hallBeforePrevState, hallLinePrevState;   // variables for reading and storing the hall sensors status

float rampFunc(float val){
  return val;
}

void setup() {
  //neo pixels 
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  allLEDS(0,0,255); //all leds to blue, show life!
  
  pinMode(HallLinePIN, INPUT_PULLUP);    
  pinMode(HallBeforeLinePIN, INPUT_PULLUP);    
  pinMode(StartButton, INPUT);  
  pinMode(LightRelayPIN, OUTPUT);   
  pinMode(StartBtnLED, OUTPUT);
  
  
  Serial.begin(9600);
  motor.setup();
  motor.setProfile(rampFunc);
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

  
  hallBeforePrevState = hallBeforeState;
  hallLinePrevState = hallLineState;
  hallBeforeState = digitalRead(HallBeforeLinePIN);
  hallLineState = digitalRead(HallLinePIN);
  

  if (hallBeforeState && !hallBeforePrevState) {  // passed sensor hallBefore
    motor.ramp(-.2,1000);
  }
  if (!hallBeforeState && hallBeforePrevState) {  // on sensor hallBefore
    motor.ramp(-.2,1000);
  }
  if (hallLineState && !hallLinePrevState) {  // passed sensor hallLine
    motor.ramp(-1,1000);
  }
  if (!hallLineState && hallLinePrevState) {  // on sensor hallBefore
    motor.ramp(0,1000);
    if (racing==1){
      lapCounter++;
    }
  }

  if (!hallLineState && !hallLinePrevState){ // still on sensor hallLine
    if (resetting && timeElapsed>4000){ 
      resetting =0;
      allLEDS(0,0,0);
      strip.setPixelColor(44, 139,0,139); // set color purple to create virtual pace car
      strip.setPixelColor(45, 139,0,139); // 
      strip.setPixelColor(46, 139,0,139); // 
      strip.setPixelColor(47, 139,0,139); // 
      strip.show();
    }
  }
  
  if (resetting == 1 && timeElapsed > 2000){ // let solar power die, then kick in motor reset.
    motor.idle();
  }else{
    motor.stop();
  }

  if (resetting == 0 && racing ==0){
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
      endRace(1);      
    }
    
    if (timeElapsed >timeToRace){
      endRace(0);
    }    
  }
}

void waitToStart(){
  digitalWrite(StartBtnLED, HIGH);  //light Start button telling visitor they can play!
  while (digitalRead(StartButton) == HIGH) { 
    // wait for user to press start button.
  }  
  racing =1;
  timingIndexLED = 0;
  resetting=0;
  timeElapsed =0;
  colorWipe(60,255,0,0); //red
  colorWipe(60,255,255,0);  //yellow
  colorWipe(60,0,255,0);  //green
  for(uint16_t i=30; i<strip.numPixels(); i++) {
    strip.setPixelColor(47-i, 0,0,0); // turn off all green LEDS     
  }
  strip.show(); 
  digitalWrite(StartBtnLED, LOW);  
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
  for(uint16_t i=30; i<strip.numPixels(); i++) {
    strip.setPixelColor(47-i, R,G,B); //color wipe
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

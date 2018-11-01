//#include "arduino.h"

#include <Wire.h>
#include <Adafruit_MotorShield.h>
static float rmp(float val){
  return val;
}

static Adafruit_MotorShield AFMS = Adafruit_MotorShield();

//test edit

class stepper {
public:
  Adafruit_StepperMotor *stepper;

  // stores the direction and pulse speed
  int dir;
  int SPR;
  float maxRPM;
  float curSpeed;
  float targSpeed;
  float startSpeed;

  unsigned long startTime;
  unsigned long refreshTime;
  unsigned long stepTime;
  int rampTime;

  bool running;
  bool ramping;

  //stores the pointer to the callback function
  void (*stopCB)();

  void (*rampCB)();

  float (*rampProfile)(float);

  //declare the actuator object and set defaults.
  void setProfile(float (*rmpPro)(float)){
    rampProfile = rmpPro;
  }
  
  stepper(int stepsPerRev, int port, float mxRPM = 60){
    SPR = stepsPerRev;
    stepper = AFMS.getStepper(stepsPerRev, port);

    maxRPM = mxRPM;
    curSpeed = 0;
    targSpeed = 0;
    startSpeed = 0;

    startTime = 0;
    rampTime = 0;
    refreshTime = 0;
    stepTime = 0;

    stopCB = 0;
    
    running = false;
    ramping = false;
    
    setProfile(rmp);
  }

  int direction(){
    return dir;
  }

  
  void setup(){
    //digitalWrite(enbPin,HIGH);
    
    AFMS.begin();
  }
  
  void stop(){
    stepper->release();
  }

  void disable(){
    stepper->release();
  }
  
  void run(float spd){
    running = (spd != 0 || ramping);
    curSpeed = spd;

    //stepTime = millis() + (1000.0 / abs(curSpeed * SPR * maxRPM));

    if(!running && !ramping){
      if(stopCB) stopCB();
      running = false;
    }
  }

  void ramp(float spd, int time, void (*rmpCB)() = NULL){
    if(targSpeed != spd){
      ramping = true;
      startTime = millis();
      rampTime = time;
      targSpeed = spd;
      startSpeed = curSpeed;
      rampCB = rmpCB;
    }
  }

  void stopRamp(){
    ramping = false;
  }

  void idle(){
    if(ramping){
      if(startTime + rampTime > millis()){
        float speedOffset = targSpeed - startSpeed;
        //Serial.println(speedOffset);
        float newSpeed = startSpeed + speedOffset * rampProfile(float(millis()-startTime)/float(rampTime));
        //Serial.println(newSpeed);
        run(newSpeed);
      } else {
        ramping = false;
        run(targSpeed);
        if(rampCB) rampCB();
      }
    }

    if(running && stepTime < millis()){
      if(abs(curSpeed*SPR*maxRPM/60)>2){
        stepTime = millis() + (1000.0 / abs(curSpeed * SPR * maxRPM/60));
        if(curSpeed > 0) stepper->onestep(FORWARD, DOUBLE);
        else stepper->onestep(BACKWARD, DOUBLE);
      } else stepTime = millis() +1;
    }
  }

};

#include "hw_timer.h"

/****************************** DIMMER SETUP ********************************************
NOTE on Sunrise/Sunset long transitions: This is coded to handle increment times based on
the set min and max brightness for the device, then a rapid fall to 0%/rapid rise to 100%,
OR a rapid movement from 100 to max brightness when falling/0 to min brightness when rising.
It is NOT coded to handle the full range: 0-100/100-0. I always sunrise to a mid-range 
percent, or have the light set to a gentle dim % when it starts the sunset. */

// Do not edit these variables

int state = 0;                  // 0 = off; 1 = on
byte tarBrightness = 0;         // target brightness used in ISR
byte curBrightness = 0;         // value controled by pwm as light transtions to target
byte zcState = 0;               // 0 = ready; 1 = processing; used in ISR
int Percent = 0;                // Percent = % Brightness
int inTransition = 0;           // 0 = No transition in progress; 1 = Rising; 2 = Falling
int incrementT = 0;             // transition time increments
int minB = 1;                   // min Brigtness
int maxB = 99;                  // max Brightness
int SkipStart = 1;              // Used in ISR to avoid flicker
unsigned long time_now = 0;     // Millis() function

void ICACHE_RAM_ATTR zcDetectISR ();
void ICACHE_RAM_ATTR dimTimerISR ();


/********************************* DIMMER SETUP & Loop ***************************************/

void dimmerSetup() {
  pinMode(zcPin, INPUT_PULLUP);
  pinMode(pwmPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(zcPin), zcDetectISR, RISING);
  hw_timer_init(NMI_SOURCE, 0);
  hw_timer_set_func(dimTimerISR);
  minB = minPercent*254/100;  // Recalculate min/max brightness value for
  maxB = maxPercent*254/100;  // time increment per step conversions
}

void dimmerLoop() {
  if (inTransition != 0) {
    if(millis() > time_now + incrementT){
      time_now = millis();
      if (curBrightness == NewBrightness) {
        inTransition = 0;
        Serial.println("Transition complete!");
      }
      else if (inTransition == 1) {
        if (curBrightness == maxB) { // Rapid finish from maxB to 100%, which will also end cycle
          inTransition = 0;
          tarBrightness = NewBrightness;
        }
        else {                       // ...or contine one step at a time
          ++tarBrightness;
        }
      }
      else if (inTransition == 2) {
        if (curBrightness == minB) { // Repeat, but rapid finish from minB to 0%
          inTransition = 0;
          tarBrightness = NewBrightness;
        }
        else {
          --tarBrightness;
        }
      }
      SkipStart = 1;
      dimTimerISR();
    }
  }
}

/************************************* DIMMER TRIGGER *****************************************/

void dimmerTrigger() {
  inTransition = 0; // Cancels active transtions
  Percent = NewBrightness*100/255;  // Calculate % brightness
  Serial.println("Set Light Data:");
  Serial.print("Percent: ");Serial.print(Percent);Serial.print("%");
  if (Percent >= 1) {
    OldBrightness = NewBrightness;  // Save brightness for toggle off/on to override HA on at 0
  }
  Serial.print(", New Target: ");Serial.print(NewBrightness);
  if (Percent == 100) {             // Calculate transtion time to 100% using maxB
    incrementT = transitionTime*1000/(curBrightness-maxB);
  }
  else if (Percent == 0) {          // Calculate time to 0% using minB
    incrementT = transitionTime*1000/(curBrightness-minB);
  }
  else {            // Calculate 1-99% new brightness based on min/max, then transition time
    NewBrightness = ((maxB-minB)*Percent/100)+minB;
    incrementT = transitionTime*1000/(curBrightness-NewBrightness);
  }
  if (incrementT < 0) {             // Megative transition time = brightness rising
    incrementT = abs(incrementT);   // Make transition time positive
    inTransition = 1;
    if (curBrightness == 0) {       // Jump to minB from 0, then use transition time...
      tarBrightness = minB;
    }
    else {
      ++tarBrightness;              // ...or start rising from current percent
    }
  }
  else {
    inTransition = 2;
    if (curBrightness == 255) {
      tarBrightness = maxB;
    }
    else {
      --tarBrightness;
    }
  }
  Serial.print(", Transition incrementT: ");Serial.println(incrementT);
  time_now = millis();               // Start first transition time timer
  SkipStart = 1;                     // to avoid flicker in dimTimerISR
  dimTimerISR();                     // Start process
}

/********************************* DIMMER ISRs ******************************************/

void dimTimerISR() {
  if (curBrightness > tarBrightness) {
    --curBrightness;
  }
  else if (curBrightness < tarBrightness) {
    ++curBrightness;
    if (state = 0) {
      state = 1;
    }
  }
  if (curBrightness <= 2) {
    state = 0;
    digitalWrite(pwmPin, 0);
  }
  else {
    if (SkipStart == 0) {
      digitalWrite(pwmPin, 1);
      zcState = 0;
    }
    else {                //****** Runs a non-changing first cycle to avoid flicker
      SkipStart = 0;
      zcState = 0;
    }
  }
}

void zcDetectISR() {
  if (zcState == 0) {
    zcState = 1;
    if (curBrightness < 250) {
      digitalWrite(pwmPin, 0);
      int dimDelay = 30 * (255 - curBrightness) + 400;//400
      hw_timer_arm(dimDelay);
    }
  }
}

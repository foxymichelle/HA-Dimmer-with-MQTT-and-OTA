#include "hw_timer.h"

/****************************** DIMMER SETUP ********************************************
NOTE on Sunrise/Sunset long transitions: This is coded to handle increment times based on
the set min and max brightness for the device, then a rapid fall to 0%/rapid rise to 100%,
OR a rapid movement from 100 to max brightness when falling/0 to min brightness when rising.
It is NOT coded to handle the full range: 0-100/100-0. I always sunrise to a mid-range 
percent, or have the light set to a gentle dim % when it starts the sunset. */

/****** Do NOT edit these variables ******/

// byte state = 0;                  // 0 = off; 1 = on - MOVED to communication page
byte tarBrightness = 0;          // target brightness used in ISR
byte curBrightness = 0;          // value controled by pwm as light transtions to target
byte zcState = 0;                // 0 = ready; 1 = processing; used in ISR
byte Percent = 0;                // Percent = % Brightness
byte inTransition = 0;           // 0 = No transition in progress; 1 = Rising; 2 = Falling
int incrementT = 0;              // transition time increments
byte minB = 1;                   // min Brigtness
byte maxB = 99;                  // max Brightness
byte SkipStart = 1;              // used in ISR to avoid flicker
unsigned long transtionTime = 0; // millis() function
bool dimISRworking = false;      // the next 3 variables check if ISRs are running when the
bool zcISRworking = false;       // dimmer is on; if there is an interupt conflict that stops
unsigned long ISRtime = 0;       // the process -which won't restart again on its own- then
                                 // the ISRs are reset to resume proper dimmer function.
void ICACHE_RAM_ATTR zcDetectISR ();
void ICACHE_RAM_ATTR dimTimerISR ();


/********************************* DIMMER SETUP & Loop ***************************************/

void dimmerSetup() {
  void ICACHE_RAM_ATTR zcDetectISR ();  // included here as well for dimmer reset (after conflict error shutdown)
  void ICACHE_RAM_ATTR dimTimerISR ();
  pinMode(zcPin, INPUT_PULLUP);
  pinMode(pwmPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(zcPin), zcDetectISR, RISING);
  hw_timer_init(NMI_SOURCE, 0);
  hw_timer_set_func(dimTimerISR);
  DEBUGLN("dimmer ISRs turned on");
  minB = minPercent*254/100;  // Recalculate min/max brightness value for
  maxB = maxPercent*254/100;  // time increment per step conversions
}

void dimmerShutdown() {       // for dmmer reset
  state = 0;
  timer1_disable();
  detachInterrupt(digitalPinToInterrupt(zcPin));
  digitalWrite(pwmPin, 0);
}

void dimmerLoop() {
  if (state == 1) {                             // Only runs if the dimmer is on
    if (currentMillis - ISRtime >= 1500){
      if (zcISRworking && dimISRworking) {      // Every 1.5s, check working bools and reset to
        ISRtime = millis();                     // false; functioning ISRs will make true again.
        zcISRworking = false;
        dimISRworking = false;
        DEBUG("* ");
      }
      else {                                    // if ISRs failed and did not make true,
        DEBUGLN("dimmer FAILED");               // then reset dimmer ISRs.
        dimmerShutdown();
        delay(100);
        dimmerSetup();
        dimmerTrigger();
      }
    }
    if (inTransition != 0 && currentMillis - transtionTime >= incrementT) {
      transtionTime = millis();
      if (curBrightness >= NewBrightness-1 && curBrightness <= NewBrightness+1) {
        inTransition = 0;
        DEBUGLN("Transition complete!");
      }
      else if (inTransition == 1) {
        if (curBrightness >= maxB) { // Rapid finish from maxB to 100%, which will also end cycle
          inTransition = 0;
          tarBrightness = NewBrightness;
        }
        else {                       // ...or contine one step at a time
          ++tarBrightness;
        }
      }
      else if (inTransition == 2) {
        if (curBrightness <= minB) { // Repeat, but rapid finish from minB to 0%
          inTransition = 0;
          tarBrightness = NewBrightness;
        }
        else {
          --tarBrightness;
        }
      }
      SkipStart = 1;                  // Stops the flicker!
      dimTimerISR();
    }
    checkMQTT();
  }
}

/************************************* DIMMER TRIGGER *****************************************/

void dimmerTrigger() {
  DEBUGLN("Start dimmerTrigger");
  inTransition = 0;                 // Cancels active transitions
  Percent = NewBrightness*100/255;  // Calculate % brightness
  DEBUGLN("Set Light Data:");
  DEBUG("Percent: ");DEBUG(Percent);DEBUG("%");
  if (Percent >= 1) {
    if (state == 0) {
      dimmerSetup();                // Get ISRs running again
    }
    OldBrightness = NewBrightness;  // Save brightness for toggle off/on to override HA on at 0
    if (Percent == 100) {           // Set 100 to slightly less than 255 to try to fix over-dimming
      NewBrightness = 245;
    }
    else {                          // Calculate 1-99% new brightness based on min/max
      NewBrightness = ((maxB-minB)*Percent/100)+minB;
    }
  }
  DEBUG(", New Target: ");DEBUG(NewBrightness);
  if (transitionTime <= 1) {
    tarBrightness = NewBrightness;
    incrementT = 0; 
  }
  else {
    if (Percent == 100) {            // Calculate transtion time to 100% using maxB
      incrementT = transitionTime*1000/(curBrightness-maxB);
    }
    else if (Percent == 0) {         // Calculate time to 0% using minB
      incrementT = transitionTime*1000/(curBrightness-minB);
    }
    else {
      incrementT = transitionTime*1000/(curBrightness-NewBrightness);
    }
    if (incrementT < 0) {            // Negative transition time = brightness rising
      incrementT = abs(incrementT);  // Make transition time positive
      inTransition = 1;
      if (curBrightness == 0) {      // Jump to minB from 0, then use transition time...
        tarBrightness = minB;
      }
      else {
        ++tarBrightness;             // ...or start rising from current percent
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
  }
  DEBUG(", Transition incrementT: ");DEBUGLN(incrementT);
  currentMillis = millis();
  transtionTime = millis();          // Start first transition timer
  ISRtime = millis();
  SkipStart = 1;                     // to avoid flicker in dimTimerISR
  dimTimerISR();                     // Start process
  DEBUGLN("End dimmerTrigger");
}

/********************************* DIMMER ISRs ******************************************/

void dimTimerISR() {
  dimTimer();
}

void dimTimer() {
  dimISRworking = true;              // In loop, checks if working
  if (curBrightness != tarBrightness) {
    if (curBrightness > tarBrightness) {
      --curBrightness;
    }
    else if (curBrightness < tarBrightness) {
      ++curBrightness;
      if (state == 0) {
        state = 1;
      }
    }
  }
  if (deviceState[1] == 'F' && curBrightness <= 2) {
    dimmerShutdown();
    DEBUGLN("Dimmer turned off - run dimmerShutdown");
  }
  else {
    if (SkipStart == 0) {
      digitalWrite(pwmPin, 1);
      zcState = 0;
    }
    else {                           // Runs a non-changing first cycle to avoid flicker
      SkipStart = 0;
      zcState = 0;
    }
  }
}

void zcDetectISR() {
  zcDetect();
}

void zcDetect() {
  zcISRworking = true;
  if (zcState == 0) {
    zcState = 1;
    if (curBrightness < 250) {
      digitalWrite(pwmPin, 0);
      int dimDelay = 30 * (255 - curBrightness) + 400;
      hw_timer_arm(dimDelay);
    }
  }
}

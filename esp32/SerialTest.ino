/*
THIS IS A WORK IN PROGRESS!

Wanting to move my dimmers to more reliable esp32 modules, I attempting to recreate the stability
and smooth dimming that I was able to achieve on the esp3266 modules. The esp32 has more capabilities
and should suffer less/no conflicts and crashes. I have tested this with wifi, mqtt, and OTA, and
everything was running smoothly with much quicker response times (vs the esp8266) when changing the 
light's percent brightness from the HA console.

By cranking up the ISR timer frequency and reducing tick count to 1, I was able to get a good, full-range
brightness spread on LEDs with virtually no flicker even at the lowest settings. The LED string of low-watt bulbs
achieved the full range between about 35-85, with it reaching 50% visible brightness around 40 - as you set higher 
percentages,there is less visible difference between each step. Below and above this range the light remains off, 
although at very low percentage the light did turn full on.

In my functional update, I plan to use min/max to represent the high and low end for a realistic spread (find
the most stable min, then choose the max so that setting 50% brightness visually represents half bright, etc.), then
I will add a fullBright variable that will be used to set 100% brightness.

The ISR timer tick count is also calculated using steps (1-255) instead of percentages. The advantage to this
is that long transitions can be done using more frequent, smaller steps/changes in brightness, which is how
my esp8266 also works.

I also plan to update MQTT to the auto-discover format.

I tested with a single channel, but the code used should work with multiple channels. Since the serial monitor
will only set the first dimmer, you can add your own code to test having it set all dimmers to the same percent
brightness, or set it to randomly add or subtract from what you entered in the serial monitor to test setting
each channel to different percent brightnesses.

*/

#include <WiFi.h>

const char* ssid = "YourNetwork";
const char* password = "YourPassword";
const byte zcPin = 12;            // your zero-cross-over pin
const byte pwmPin[] = {13};       // your pwm pin for each Channel (list all pins separated by a comma)
const byte channels = 0;          // total number of channels counting from 0 (ex: 0 = 1 channel or 3 = 4 channels)

hw_timer_t *dim_timer = NULL;     // Define ESP32 timer
const byte pulseWidth = 1;        // Used in dimTimerISR

/*  For multiple channels, add to the volatile variables list. 
    So 4 channels would look like {0,0,0,0} or {1,1,1,1} */

volatile byte dimPower[] = {0};        // Target % brightness
volatile byte zeroCross[] = {0};       // Prevents dimTimerISR actions unless zcDetect ISR has triggered
volatile byte dimState[] = {1};        // 0 = OFF; 1 = ON
volatile int dimCounter[] = {0};       // Counts each dimTimerISR trigger; used by dimPulse
volatile byte dimPulse[] = {1};        // Set to inverse of % brightness to determine dimCounter counts to create PWM intervals

#define CONFIG_ESP_TIMER_INTERRUPT_LEVEL  3
void ICACHE_RAM_ATTR zcDetectISR ();
void ICACHE_RAM_ATTR dimTimerISR ();

#define serial_baud     115200
#define USE_SERIAL  Serial

int outPer = 0;
int outVal = 0;



/********************************* DIMMER SETUP & Loop ***************************************/

void setup() {
  USE_SERIAL.begin(serial_baud);
  delay(100);
  WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting");

    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(100);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
  USE_SERIAL.println("Dimmer Program is starting...");
  USE_SERIAL.println("Set value");
  pinMode(zcPin, INPUT_PULLUP);                                  // assign ZC pin as input
  attachInterrupt(digitalPinToInterrupt(zcPin), zcDetectISR, RISING);
  dim_timer = timerBegin(1, 3200, true);                         // create timer for dimTimerISR
  timerAttachInterrupt(dim_timer, &dimTimerISR, true);           // attach timer to dimTimerISR
  timerAlarmWrite(dim_timer, 1, true);                          // fire dimmerISR every interrupt_interval_us
  timerAlarmEnable(dim_timer);                                   // enable timer for dimTimerISR
  for (int i = 0; i <= channels; i++) {
    pinMode(pwmPin[i], OUTPUT);                                  // assign pwm pins as output
    digitalWrite(pwmPin[i], 0);                                  //initial state of channel is off
  }
}
void loop() {
  int prePer = outPer;

  if (USE_SERIAL.available())
  {
    int buf = USE_SERIAL.parseInt();
    if (buf != 0) outPer = buf;
  }
  outVal = outPer*255/100;
  dimPower[0] = outPer; // setPower(0-255), then convert to % brightness;
  dimPulse[0] = 256-outVal;  // invert % brightness
  if (prePer != outPer)
  {
    USE_SERIAL.print("Set to: ");
    USE_SERIAL.print(dimPower[0]);USE_SERIAL.println("%");
  }
}

/********************************* DIMMER ISRs ******************************************/

void dimTimerISR() {
  for (int k = 0; k <= channels; k++) {
		if (zeroCross[k] == 1 ) {
			dimCounter[k]++;
			if (dimCounter[k] >= dimPulse[k]-2 && dimPulse[k] != 100 ) {
				digitalWrite(pwmPin[k], 1);	
			}
			if (dimCounter[k] >= (dimPulse[k]-2 + pulseWidth) ) {
				digitalWrite(pwmPin[k], 0);
				zeroCross[k] = 0;
				dimCounter[k] = 0;
			}
		}
	}
}

void zcDetectISR() {
  for (int i = 0; i <= channels; i++ ) 
		if (dimState[i] == 1) {
			zeroCross[i] = 1;
		}
}

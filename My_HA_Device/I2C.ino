/* As a shared file for all devices, the global variables will apply to all devices;
only code within functions can be wrapped in if statements to run on only the
devices with the specified modules. If another device ends up with a new module
that requres a pin already labeled, simply use the initial pin label, give the
variable a more generic/shared name, or use a new variable with same pin... should work! */

#include <AHTxx.h>
AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR);

/********************************* I2C SETUP *******************************************
NOTE: On Wemos, I2C only works using 5V to Vin (cannot use 3.3V)
Working SCL/SDA combinations on Wemos: GPIO5/4 (D1/D2)
Working SCL/SDA combinations on NodeMCU: GPIO5/4 (D1/D2) ; 0/2 (D3/D4) */

/****** Edit these variable ******/

const byte sclPin = 5;      // D1
const byte sdaPin = 4;      // D2
const int ReadInterval = 30000;     // Set how often to read/send / 2; toggles btween 
                                    // reading humidity and reading temp + send

/****** Do NOT edit these variables ******/

float Temp = 0;
float CheckTemp = 0;
float SavedTemp = 0;
float Humi = 0;
float CheckHumi = 0;
float SavedHumi = 0;
byte IntervalC = 0;
byte readToggle = 0;         // 0 = read humidity, 1 = read temp + send data
unsigned long lastRead = 0;
bool connectBus = true;

/****************************** SETUP & LOOP *******************************************/

void busSetup() {
  if (DeviceID != 0) {            /****** Wrap setup & loop to isolate per device; ex (DeviceID == 1) if bus is on device 1 only ******/
    Wire.begin(sdaPin, sclPin);
    if (aht10.begin()) {
      DEBUGLN(F("AHT10 OK"));
      connectBus = false;
      Wire.setClock(100000);             // experimental I2C speed! 400KHz, default 100KHz
      Wire.setClockStretchLimit(1000);
    }
    else {
      DEBUGLN(F("AHT1x not connected or fail to load calibration coefficient"));
      lastRead = millis();        // time stamp to trigger try again later in loop
    }
  }
}

void aht10Reset() {
  aht10.softReset();
}

void busLoop() {
  if (DeviceID != 0) { 
    if (currentMillis - lastRead >= ReadInterval) {
      if (connectBus) {
        busSetup();
      }
      else {
        lastRead = millis();
        if (readToggle == 0) {
          readToggle = 1;
          // yield();
          float Humi = aht10.readHumidity();
          CheckHumi = Humi;
          DEBUG("Humidity: "); DEBUG(CheckHumi); DEBUGLN(" \%");
        }
        else {
          readToggle = 0;
          ++IntervalC;
          // yield();
          float Temp = aht10.readTemperature();
          CheckTemp = (Temp * 18.0)/ 10.0 + 32.0;     // Convert C to F
          DEBUG("Temp: "); DEBUG(CheckTemp); DEBUGLN(" F");
          if (CheckTemp>SavedTemp+.1 || CheckTemp<SavedTemp-.1 || CheckHumi>SavedHumi+.1 || CheckHumi<SavedHumi-.1 || IntervalC == 8) {
            IntervalC = 0;
            if (CheckTemp < 150 || CheckTemp > 10) {  // If using sensor graph in HA, the extreme 500 degree error
              SavedTemp = CheckTemp;                  // outlier destroys the graph view, so don't send that value.
              SavedHumi = CheckHumi;                  // *** IT'S STILL SENDING, SO FIX ON NEXT UPDATE! ***
            }
            else {
              if (SavedTemp < 150) {
                SavedTemp = SavedTemp - 10; // This sudden drop 10 degrees is an obvious error to see on the HA graph.
              }
              else {
                SavedTemp = 55;             // If error is on boot/reboot
              }
            }
            sendBus = true;
          }
        }
      }
    }
  }

/************************************ MQTT SEND DATA *****************************************/

void sendBusData() {
  char buffer[256];
  StaticJsonDocument<256> doc;
  doc["temperature"] = SavedTemp;
  doc["humidity"] = SavedHumi;
  size_t n = serializeJson(doc, buffer);
  DEBUG("Publish: ");
  DEBUGLN(buffer);
  client.publish(aht10Topic, buffer, n);
}

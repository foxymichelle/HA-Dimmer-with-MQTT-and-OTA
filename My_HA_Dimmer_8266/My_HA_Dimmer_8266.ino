/********************************* HA & MQTT CONNECTION ***************************************/

#define serial_baud     115200
const char* ssid = "YourWifi";
const char* password = "WifiPassord";
const char* mqtt_server = "YourHAIP";       // IP of the MQTT broker/home assisstant server
const char* mqtt_username = "username";   // HA user created for Mosquitto Broker add-on
const char* mqtt_password = "password";   // HA user password

/********************************** MULTIPLE DEVICE SETUP  ***********************************
Maintian one file, then simply change the DeviceID before updating to each madule.

For DEVICE_1, etc, give the device a friendly name. If you have more than one entity per device,
you can specify those names in your HA config.yaml*/

/******!! ALWAYS EDIT THIS VARIABLE before uploading updates to each device !!******/

const byte DeviceID = 1;  // ***IMPORTANT*** Which device will be loaded on this module? (1 for DEVICE_1, etc.)

/****** Edit these variable ******/

const byte zcPin = 12;              // your zero-cross-over pin
const byte pwmPin = 13;             // your pwm pin
const char* LIGHT_1 = "LR Device";  // Living Room device name
const char* LIGHT_2 = "Kitchen";    // Kitchen device name
const char* LIGHT_3 = "Primary";    // Primary room device name
const char* LIGHT_4 = "Patio";      // Patio device name
// Add or remove lights as needed
unsigned long currentMillis = 0;

/****** Do NOT edit these variables (edit below is setup per device instead) ******/

const char* mqttName = "xx";
const char* availablityTopic = "xx";
const char* dimmerTopic = "xx";
const char* dimmerCommand = "xx";
const char* deviceTopic = "xx";         // Unique switch to notify HA of reconnect
const char* deviceCommand = "xx";       // When swtiched on in HA, reboots device
int minPercent = 0;
int maxPercent = 0;

/********************************* SETUP & LOOP **********************************************
The collection of files may contain only ONE setup and loop function. Those are
located here, triggering setup and loop functions in each file. 

For each dimmer device, define the specific values below:
  - ignore mqtt name - it sets it to the name identified above
  - MQTT dimmer topic and command
  - min & maxPercent - test how your lights respond at the default 1-99%, note optimal
    low value and optimal high value and enter those below. When you set the light to 1-99%,
    the actual bightness produced will fall in your desired range. 
    NOTE that 0 is always off; 100 is alwasy full on, despite the range you specify) */

void setup() {
  currentMillis = millis();
  if (DeviceID == 1) {
    mqttName = DEVICE_1;                     // Do NOT edit (uses variable name already created)
    availablityTopic = "livingroom/availablity";
    dimmerTopic = "livingroom/dimmer";       // MQTT topic string
    dimmerCommand = "livingroom/dimmer/set"; // ALWAYS use topic string + "/set" (how device filters messages)
    minPercent = 1;                          // Specify device min...
    maxPercent = 99;                         // ...and max after testing
    deviceTopic = "livingroom/devicestate";
    deviceCommand = "livingroom/devicestate/set";
  }
  else if (DeviceID == 2) {
    mqttName = DEVICE_2;
    dimmerTopic = "kitchen/dimmer";
    availablityTopic = "kitchen/availablity";
    dimmerCommand = "kitchen/dimmer/set";
    minPercent = 1;
    maxPercent = 99;
    deviceTopic = "kitchen/devicestate";
    deviceCommand = "kitchen/devicestate/set";
  }
  else if (DeviceID == 3) {
    mqttName = DEVICE_3;
    dimmerTopic = "primary/dimmer";
    availablityTopic = "primary/availablity";
    dimmerCommand = "primary/dimmer/set";
    minPercent = 1;
    maxPercent = 99;
    deviceTopic = "primary/devicestate";
    deviceCommand = "primary/devicestate/set";
  }
  else if (DeviceID == 4) {
    mqttName = DEVICE_4;
    dimmerTopic = "patio/dimmer";
    availablityTopic = "patio/availablity";
    dimmerCommand = "patio/dimmer/set";
    minPercent = 1;
    maxPercent = 99;
    deviceTopic = "patio/devicestate";
    deviceCommand = "patio/devicestate/set";
  }
  // Add to or remove the above "else if..." segements for each device as needed
  wifiSetup();
  dimmerSetup();      // Start timers in dimmerSetup last (this must run last in sequence)
}

void loop() {
  currentMillis = millis();
  wifiLoop();
  dimmerLoop();
}

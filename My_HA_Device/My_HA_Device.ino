/********************************* HA & MQTT CONNECTION ***************************************/

#define serial_baud     115200
const char* ssid = "YourWifi";
const char* password = "WifiPassord";
const char* mqtt_server = "YourHAIP";       // IP of the MQTT broker/home assisstant server
const char* mqtt_username = "username";   // HA user created for Mosquitto Broker add-on
const char* mqtt_password = "password";   // HA user password

/********************************** MULTIPLE DEVICE SETUP  ***********************************
Maintian one file, then simply change the DeviceID before updating to each madule. */

int DeviceID = 1;  // **** IMPORTANT ****: ID device before uploading (1 = LIGHT_1, etc.)

// Edit the items below

const byte zcPin = 12;              // your zero-cross-over pin
const byte pwmPin = 13;             // your pwm pin
const char* LIGHT_1 = "LR Device";  // Living Room device name
const char* LIGHT_2 = "Kitchen";    // Kitchen device name
const char* LIGHT_3 = "Master";     // Master room device name
const char* LIGHT_4 = "Patio";      // Patio device name
// Add or remove lights as needed

// DO NOT EDIT the items below (edit these in the setup function below instead!)

const char* mqttName = "xx";
const char* dimmerTopic = "xx";
const char* dimmerCommand = "xx";
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
  if (DeviceID == 1) {
    mqttName = LIGHT_1;                      // Do NOT edit (uses variable name created above)
    dimmerTopic = "livingroom/dimmer";       // MQTT topic string
    dimmerCommand = "livingroom/dimmer/set"; // ALWAYS use topic string + "/set"
    minPercent = 1;                          // Specify device min...
    maxPercent = 99;                         // ...and max after testing
  }
  else if (DeviceID == 2) {
    mqttName = LIGHT_2;
    dimmerTopic = "kitchen/dimmer";
    dimmerCommand = "kitchen/dimmer/set";
    minPercent = 1;
    maxPercent = 99;
  }
  else if (DeviceID == 3) {
    mqttName = LIGHT_3;
    dimmerTopic = "master/dimmer";
    dimmerCommand = "master/dimmer/set";
    minPercent = 1;
    maxPercent = 99;
  }
  else if (DeviceID == 4) {
    mqttName = LIGHT_4;
    dimmerTopic = "patio/dimmer";
    dimmerCommand = "patio/dimmer/set";
    minPercent = 1;
    maxPercent = 99;
  }
  // Add or remove "else if..." segements for each device as needed
  wifiSetup();
  dimmerSetup();
}

void loop() {
  wifiLoop();
  dimmerLoop();
}

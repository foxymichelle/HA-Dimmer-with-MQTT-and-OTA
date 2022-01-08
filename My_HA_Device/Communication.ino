#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);

// Do not edit these variables
bool reboot = true;
const char* deviceState = "OFF";// Used for MQTT / Command
int transitionTime = 1;         // Defaults to 1 when transition time not received
int NewBrightness = 0;          // Brightness = value between 1-256
int OldBrightness = 0;          // Resets brightness to last state when light toggled off/on

/************************* SETUP - triggered in main setup function **************************/

void wifiSetup() {
  Serial.begin(serial_baud);
  wifiConnect();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  // OTA Stuff
  if (DeviceID == 1) {
    ArduinoOTA.setHostname("Living Room Device");
  }
  else if (DeviceID == 2) {
    ArduinoOTA.setHostname("Kitchen Device");
  }
  else if (DeviceID == 3) {
    ArduinoOTA.setHostname("Foxy Device");
  }
  else if (DeviceID == 4) {
    ArduinoOTA.setHostname("Patio Device");
  }
  ArduinoOTA.onStart([]() {
    timer1_disable();                   /************** Stops timer interupts for OTA ***********/
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { 
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

/**************************** LOOP - triggered in main setup function ************************/
void wifiLoop() {
  if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
}

/************************************ WIFI CONNECT *****************************************/
void wifiConnect(){
  Serial.println("Connecting to wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 
}

/************************************** MQTT CONNECT *****************************************/

void reconnect(){
  if (client.connect(mqttName, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker");
    client.subscribe(dimmerTopic);
    client.subscribe(dimmerCommand);
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
    // Set up a 5000ms countdown to try again in 5 seconds?
  }
}

/*********************************** MQTT CALLBACK *******************************************/

void callback(char* topic, byte* payload, unsigned int length) {
  String Topic = String(topic);              // Check if the word "set" is in the
  int checkTopic = Topic.indexOf("set");     // topic; filters out the device's
  if (checkTopic != -1) {                    // published/confirmation messages (sendState).
    Serial.print("Message arrived from ");
    Serial.println(topic);
 
    StaticJsonDocument <256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    deviceState = doc["state"];
    transitionTime = doc["transition"];
    NewBrightness = doc["brightness"];

    if (transitionTime <= 1 || reboot) {
      reboot = false;
      transitionTime = 1;
    }
    if (deviceState[1] == 'N' && NewBrightness == 0) {
      NewBrightness = OldBrightness;
    }
    Serial.print("Command: {state: "); Serial.print(deviceState);
    Serial.print(", transition: "); Serial.print(transitionTime);
    Serial.print(", brightness: "); Serial.print(NewBrightness); 
    Serial.println("}");
    sendState();     // First send a message back to confirm message received,
    dimmerTrigger(); // then trigger command, which will adjust for min/max brightness
  }
}

/*********************************** MQTT SEND STATE *****************************************/

void sendState() {
  char buffer[256];
  StaticJsonDocument<256> doc;
  doc["state"] = deviceState;
  doc["transition"] = transitionTime;
  doc["brightness"] = NewBrightness;
  size_t n = serializeJson(doc, buffer);
  Serial.print("Publish: ");
  Serial.println(buffer);
  client.publish(dimmerTopic, buffer, n);
}

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);

#define DEBUGSTATE 0 // 1 on; 0 off - turn off when not using serial monitor to debug
#if DEBUGSTATE == 1
#define DEBUG(x) Serial.print(x)
#define DEBUGLN(x) Serial.println(x)
#else
#define DEBUG(x)
#define DEBUGLN(x)
#endif

/****** Do NOT edit these variables ******/

const char* deviceState = "OFF"; // Used for MQTT / Command
byte transitionTime = 1;         // Defaults to 1 when transition time not received
byte NewBrightness = 0;          // Brightness = value between 1-256
byte OldBrightness = 0;          // Resets brightness to last state when light toggled off/on
bool firstBoot = true;           // Used to reset dimmer timers/interrupts on reconnect & cancel long transition
const char* wifiState = "OFF";   // Used for MQTT / Command - notify wifi reconnect / force restart
unsigned long reconnectStart = 0;         // Millis used for both wifi timeout and reconnect intervals
const int wifiInterval = 16000;           // Gives 8 seconds to connect to wifi
const int connectInterval = 60000;        // Wait 60 seconds before trying again
bool sendBus = false;
byte state = 0;                  // dimmer on or off - only checks for OTA when off!


/************************* SETUP - triggered in main setup function **************************/

void wifiSetup() {
  Serial.begin(serial_baud);
  delay(100);
  DEBUGLN("Starting setup...");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  wifiConnect();
  
  // OTA Stuff
  if (DeviceID == 1) {
    ArduinoOTA.setHostname(DEVICE_1);
  }
  else if (DeviceID == 2) {
    ArduinoOTA.setHostname(DEVICE_2);
  }
  else if (DeviceID == 3) {
    ArduinoOTA.setHostname(DEVICE_3);
  }
  else if (DeviceID == 4) {
    ArduinoOTA.setHostname(DEVICE_4);
  }
  ArduinoOTA.onStart([]() {
    timer1_disable();                              /****** Stops timer interupts for OTA ******/
    detachInterrupt(digitalPinToInterrupt(zcPin));
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
  if (WiFi.status() != WL_CONNECTED && currentMillis - reconnectStart >= connectInterval) {
    WiFi.disconnect();
    // dimmerShutdown();       // created for testing; not needed but saved for reference
    delay(500);
    wifiConnect();
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      if (currentMillis - reconnectStart >= connectInterval) {
        reconnect();
      }
    }
    if (state == 0) {           // *** ONLY checks for OTA updates when dimmer is OFF!! Update will fail if dimmer is on ***
      ArduinoOTA.handle();
    }
  }
  checkMQTT();
}

void checkMQTT() {
  if (WiFi.status() == WL_CONNECTED) {
    client.loop();
  }
}

/************************************ WIFI CONNECT *****************************************/

void wifiConnect() {
  DEBUGLN("Connecting to wifi");
  currentMillis = millis();
  reconnectStart = millis();   // Set "now" for connection timeout
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG(".");
    if (currentMillis - reconnectStart >= wifiInterval) {  // if timeouts, stop process and wait to try again
      WiFi.disconnect();
      DEBUGLN("Wifi failed to connect");
      //ESP.restart();
      break;
    }
  }
  reconnectStart = millis();   // Reset "now" for loop
  if (WiFi.status() == WL_CONNECTED) {
    DEBUGLN("WiFi connected");
    DEBUG("IP address: ");
    DEBUGLN(WiFi.localIP());
    reconnect();               // Connect MQTT to avoid the bypass wait interval in loop
    wifiState = "OFF";         // will turn back ON and try to update and verify the new
    sendwifiNotice();          // connection state with the device.
    if (!firstBoot) {
      dimmerSetup();           // dimmer interupts only need reset if reconnecting to internet.
      dimmerTrigger();
    }
  }
}

/************************************** MQTT CONNECT *****************************************/

void reconnect() {
  if (client.connect(mqttName, mqtt_username, mqtt_password, availablityTopic, 2, true, "offline")) {
    DEBUGLN("Connected to MQTT Broker");
    client.publish(availablityTopic, "online", true);
    client.subscribe(dimmerTopic);
    client.subscribe(dimmerCommand);
    client.subscribe(deviceTopic);
    client.subscribe(deviceCommand);
  }
  else {
    DEBUGLN("Connection to MQTT Broker failed...");
  }
  reconnectStart = millis();
}

/*********************************** MQTT CALLBACK *******************************************/

void callback(char* topic, byte* payload, unsigned int length) {
  String Topic = String(topic);              // Check if the word "set" is in the
  int checkTopic = Topic.indexOf("set");     // topic; filters out the device's
  if (checkTopic != -1) {                    // sent messages.
    DEBUG("Message arrived from ");
    DEBUGLN(topic);

    StaticJsonDocument <256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      DEBUG(F("deserializeJson() failed: "));
      DEBUGLN(error.c_str());
      return;
    }
    
    int checkTopic = Topic.indexOf("dimmer");     // Check if dimmer
    if (checkTopic != -1) {
      deviceState = doc["state"];
      transitionTime = doc["transition"];
      NewBrightness = doc["brightness"];

      if (transitionTime <= 1 || firstBoot) {
        firstBoot = false;
        transitionTime = 1;
      }
      if (deviceState[1] == 'N' && NewBrightness == 0) {
        NewBrightness = OldBrightness;
      }
      DEBUG("Command: {state: "); DEBUG(deviceState);
      DEBUG(", transition: "); DEBUG(transitionTime);
      DEBUG(", brightness: "); DEBUG(NewBrightness); 
      DEBUGLN("}");
      sendState();     // First send a message back to confirm message received,
      dimmerTrigger(); // then trigger command, which will adjust for min/max brightness
    }
    else {                    // Not dimmer; confirming device connection or triggering reboot
      wifiState = doc["state"];
      DEBUG("Command: {state: "); DEBUG(wifiState);
      DEBUGLN("}");
      if (wifiState[1] == 'N') {
        sendwifiNotice();
      }
      else {
        sendwifiNotice();   // First send a message back to confirm message received,
        delay(1000);
        ESP.reset();
      }
    }
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
  DEBUG("Publish: ");
  DEBUGLN(buffer);
  client.publish(dimmerTopic, buffer, n);
}

void sendwifiNotice() {
  char buffer[256];
  StaticJsonDocument<256> doc;
  doc["state"] = wifiState;
  size_t n = serializeJson(doc, buffer);
  DEBUG("Publish: ");
  DEBUGLN(buffer);
  client.publish(deviceTopic, buffer, n);
}

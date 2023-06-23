/****** Edit this variable ******/

const int pirPin = D5;

/****** Do NOT edit these variables ******/

int pirVal = LOW;          // reaads pin input
int pirState = LOW;        // updates state

/****************************** SETUP & LOOP *******************************************/

void sensorSetup() {
  if (DeviceID == 0) {            /****** Wrap setup & loop to isolate per device ******/
    pinMode(pirPin, INPUT_PULLUP);          // declare sensor as input
    digitalWrite(pirPin, HIGH);             // turn on input
  }
}

void sensorLoop() {
  if (DeviceID == 0) {
    pirVal = digitalRead(pirPin);       // read input value
    if (pirVal == HIGH) {               // check if motions detected
      if (pirState == LOW) {            // check if this was initial state change
        pirState = HIGH;
        DEBUGLN("Motion detected!");
        sendSensorData();
      }
    }
    else {                              // meaning if pirVal == LOW (no motion)
      if (pirState == HIGH) {           // check if this was the intial state change
        pirState = LOW;
        DEBUGLN("Motion ended!");
        // add a wait loop to send motion ended after my desired wait period
      }
    }
  }
}

/************************************ MQTT SEND DATA *****************************************/

void sendSensorData() {
  char buffer[256];
  StaticJsonDocument<256> doc;
  if (pirState == HIGH) {
    doc["motion"] = "ON";
  }
  else {
    doc["motion"] = "OFF";
  }
  size_t n = serializeJson(doc, buffer);
  DEBUG("Publish: ");
  DEBUGLN(buffer);
  client.publish(pirTopic, buffer, n);
}

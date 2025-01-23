<div><h3 align="center">HA-Dimmer-with-MQTT-and-OTA</h3>

  <p align="center">
    A Home Assistant Dimmer that uses MQTT (Json v6)<br>and includes ArudinoOTA for updating.
    <br />
  </p>
</div>

Added "My_HA_Dimmer_8266" for 8266 devices dedicated to the dimmer (no sensors).

### Materials

* Wemos D1 Mini with ESP8266
* PWM AC Light Phase Dimmer 110V-220V by RobotDyn
* Necessary wires and housing

### Prerequisites

* Home Assistant
* Mosquitto MQTT Add-on setup
  - Finding clear, step-by-step documentation to get MQTT up and running in HA was a painful process. For newbies, here's a great tutorial with pics! https://opencloudware.com/sht31-d-based-mqtt-sensor-for-home-assistant-with-esp32-and-toit/

### Installation & Setup

1. Download the project file. 
2. Open any ino file. All 5 should documents should be present in tabs.
3. Follow the directions in the My_HA_Device tab to update with your information.
4. Flash to your device. Once this is done, you can use select the device in your port list and update OTA.
5. In Home Assistant, add the following to your `config.yaml`
   ```yaml
    mqtt:
      light:
        - schema: json
          name: String
          unique_id: string_mqtt
          state_topic: "livingroom/dimmer"
          command_topic: "livingroom/dimmer/set"
          availability:
            - topic: "livingroom/availablity"
          brightness: true
          supported_color_modes: ["brightness"]
          retain: true
   ```
NOTE: This yaml is working as of Jan 2025, although there are warnings that I ignore after schema, brightness, and supported_color_modes. You can change the name to your own friendly anme and the unique id, plus change your topics to match whatever you used in your arduino code.

6. Test your device using the HA interface. Find your ideal 1-99% range, update the file, then reflash your device.

### Notes & Features

* This uses the old hw_timer.h library. I edited the original RobotDyn repository way back when - fixing some flickering on change, right before turning off, etc. when setting the device up with ESPAlexa. I found that my setup still produces smoother transitions, more stable lighting (less flickering), and consistent percentage results over their current repository and the ESPHome beta setup.
  - Regarding consistent results, one of the issues with the other setups is that turning the light on to a percent produces far brighter results than dimming down to the same percent - to the extreme that it was uncomfortable as a nightlight or useless to run a long sunrise transition in the morning.
  - I made some changes to the original library that fixed some of the issues back then, such as the light flickering bright when a change is first initiated, right before reaching off at the end of the transition, or when turning on from off. 
* Includes ArduinoOTA
  - Trying to get this to work, I found a number of forums and articles addressing the problem with hw_timer and ISRs interrupting the OTA and causing it to fail. None offered working solutions. I finally discovered that simply adding “timer1_disable();” under “ArduinoOTA.onStart” stopped the interrupts so that the OTA can do its thing! Huzzah!
* User specified 1-99% min and max brightness
  - The dimmer module recommends not using with LED bulbs, probably because they draw a lighter load and do best using a much smaller dimming window. When specifying my min and max brightness, I aim for the lowest, stable dim and ideally a 25, 50 and 75% that produces those visual results - so 50% actually looks like it's half as bright as full on.
  - ESPHome lets you define the 1-100% gradient, which means 100% is the defined max. I’ve always enjoyed my specified 1-99% gradient while still being able to throw the light to full brightness when needed. 
* Supports transitions:
  - I love my slow sunrises and sunsets! I had coded this feature into my old ESPAlexa to run those effects well before Alexa added their transition feature to their app (2% = long sunrise; 3% = sunset; 6% = 5 minutes timer, then sunset… and so on for longer reading timers at bedtime). I have updated the code here to instead calculate and run whatever transition time you set in HA.
  - Want a bedtime timer, then sunset? I used a "Bedtime" number helper, a script that sets the light to my reading percent, waits for the Bedtime length, then runs a long transition to off and resets the Bedtime timer to 0. I have an Automation that starts the script when Bedtime changes from 0 to any other time.
  - Longer transitions are calculated based on min/max, then naturally transition to off or 100% on if that is the end target. When the min is 20%, it's nice that it actually hits that almost almost off towards the end of the transition, then transtions quickly the rest of the way to off at the end.
  - Transitions are canceled if a new command is received; default is 1 second.
* Other features:
  - HA sets brightness to 0 when you toggle the light off, so my program saves the old percentage when turning the light off - therefore, if you toggle the light back on without setting a new percentage - in which HA sends brightness as 0 again - the program will remember your last percentage and return it to that state, as well as respond to HA with that brightness so that your HA light status shows that percent.
  - "Retain" in your yaml code saves the last message, so your light will reset to its previous state if it reboots. NOTE: On reboot, transitions default back to 1, so if your last setting was a long sunrise, it would reset to the final percent without the transition. Also, If you toggle the light from off to on, utilizing the program's saved brightness to reset… and THEN the device reboots, the retained mqtt message will report the last brightness 0 - so the light will not return to its previous state. I would be happy to update if someone can help me include a retain flag on the device’s published message, which I didn't spend too much time trying to figure out (my initial research all lead to failed attempts).
  - Have more than one dimmer device? You can set them all up in this one file, then simply change the deviceID variable before updating each device!
 
### 6/2023 Updates

* Includes AHT10 temp/hum sensor with code tweaked to diminish interrupt conflicts that result in the light turning off. While checking the AHT10 to update temp/hum is quick, if the dimmer interrupt is timed just right, at could result in a dimmer failure. Some tweaks were made to the dimmer ISRs and how the bus reading and sending data through MQTT were handled. The loop also looks for this error and will reset the dimmer when an dimmer shutdown occurs. The other solution is to try another temp/hum module that is less likely to cause issues (the AHT10 can take a while to read) or give the dimmer a dedicated device so that there are no conflicts. I personally was ok with this happening 0-3 times a day given the particular lights I have attached to these dimmers.
* Includes PIR sensor, although I never actually got around to adding that to my devices. It could easily be tweaked to add your preferred form of light control, though. I only accassionally wish I had manual control, so it hasn't been a priority.
* Device availablitiy and reboot topics for HA. Before I created the conflict error discovery and fix in the arduino code, I set up HA to trigger a device reboot if the device dimmer became unavailable. Still good to have, so I didn't delete them.
* Device continues to work if wifi or mqtt are down. The old code would continually try to connect until it failed, then would require reboot to try again. The new code sets a time limit to connect, then quits, returns to the loop, and tries again later.
* Debug lines - see #define DEBUGSTATE in Communication.ino to turn on or off.
* OTA - I discovered that it only works if the dimmer is turned off (dimmer interupts break the update). If you are updating your device, be sure to turn the dimmer. The code was updated so that it won't even look for updates in the loop if the dimmer is on.

<div><h3 align="center">How to set Min, Max, and Full Brightness in HA</h3>

  <p align="center">
    Depending on what you're powering, your dimmer may not function well in the full 1-100% dimming spread. Furthermore, if you use the full working spread,
    you will find that 50% brightness delivers something far brighter that half bright. Hence three settings: min and max to create a good representation
    of 1-99% and full to still achieve maximum, 100% brigtness. Visually, the lost values between max and full are minimal and not likely to be missed in day-to-day use.
    <br />
  </p>
</div>

### Materials & Prerequisites

* HA-Dimmer-with-MQTT-and-OTA set up on an esp32 and communicating with Home Assistant

### Home Assistant Setup

1. Once your esp32 is communicating with HA and your light entity has been created through auto discovery, you can manually add your mqtt number entities to edit min, max, and full percent values. (NOTE: number entities cannot be created through auto discovery)
2. In Home Assistant, add the following (for each dimmer channel) to your `config.yaml`, editing to match the data you created on your esp32 device.
   ```yaml
   mqtt:
    number:
    - unique_id: your_light_min
      name: "Your Light Min"
      state_topic: "yourlight/min/state"
      command_topic: "yourlight/min/set"
      icon: mdi:arrow-down-bold-circle-outline
      min: 1
      max: 99
      retain: true
    - unique_id: your_light_max
      name: "Your Light Max"
      state_topic: "yourlight/max/state"
      command_topic: "yourlight/max/set"
      icon: mdi:arrow-up-bold-circle-outline
      min: 1
      max: 99
      retain: true
    - unique_id: your_light_full
      name: "Your Light Full"
      state_topic: "yourlight/full/state"
      command_topic: "yourlight/full/set"
      icon: mdi:dots-circle
      min: 1
      max: 100
      retain: true
   ```
3. In developer tools, reload "Manually Configured MQTT Entities."
4. When the esp32 boots, the min, max, and full percents were set to 1, 99, and 100 respectively. You can now use the HA UI to test the brightenss of your light. Set min to the dimmest stable percent; full to the brightest stable percent; max to a value that, when paired with min, delivers the expected brightness at 50% and/or other values you commonly use.
5. For example: My light's dimmest stable brightness is 20% and the brightest is 80%. LED's tend to get bright very fast, though, so as the percent brightness increases, the visible change per step decreases. Through trial and error, I find that 42% delivers the best visual half brightness, so I set the max to 64%.
6. Once the values are set, all future light settings will be calculated based on them:
   * In HA, setting your light to 1% will be recalculated to 20%;
   * setting your light to 99% will be recalculated to 64%;
   * setting your light to 50% will be recalculated to 42% (middle of 20 and 64;
   * setting your light to 100% will be recalculated to 80%

NOTE: When the esp32 device is rebooted, it will fetch the last retained messages from HA to set min, max, and full to your saved settings before turning the light back on to its last setting.

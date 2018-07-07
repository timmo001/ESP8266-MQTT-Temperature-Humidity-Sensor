# ESP8266 MQTT Temperature Humidity Sensor

[![pipeline status](https://gitlab.com/timmo/ESP8266-MQTT-Temperature-Humidity-Sensor/badges/master/pipeline.svg)](https://gitlab.com/timmo/ESP8266-MQTT-Temperature-Humidity-Sensor/commits/master)

ESP8266 MQTT Temperature Humidity Sensor

## Hardware Example

![Wemos D1 Mini Sensor](diagrams/wemos_d1_sensor.svg)

## Software Setup

- Using Atom or VS Code, install [Platform IO](https://platformio.org/platformio-ide)
- Once setup, install the `esp8266` embedded platform
- Rename `src/setup-template.h` to `src/setup.h` and add your network, MQTT and
  lighting setup information. Take note of the `deviceName` you set. You will
  need this later to subscribe to MQTT messages.
- Build the project (Ctrl+Alt+B) and check for any errors

  > If the build produces an error referencing dependencies, You will need to
  > manually install these libraries:
  - ArduinoJson
  - PubSubClient
  - Adafruit Unified Sensor
  - DHT sensor library

- Upload to your board of choice (Ctrl+Alt+U). This project was created
  specifically for the `Wemos D1 Mini` but can be configured to work
  with another WiFi board (`NodeMCU` etc.) with some tinkering.

## Example Home Assistant Configuration

```yaml
sensor:
    # Temperature
  - platform: mqtt
    state_topic: "sensors/dht22001"
    name: "DHT22 01 Temperature"
    unit_of_measurement: "°C"
    value_template: '{{ value_json.temperature | round(2) }}'
    # Temperature (Feels Like)
  - platform: mqtt
    state_topic: "sensors/dht22001"
    name: "DHT22 01 Temperature (Feels Like)"
    unit_of_measurement: "°C"
    value_template: '{{ value_json.heatIndex | round(2) }}'
    # Humidity
  - platform: mqtt
    state_topic: "sensors/dht22001"
    name: "DHT22 01 Humidity"
    unit_of_measurement: "%"
    value_template: '{{ value_json.humidity | round(2) }}'
```

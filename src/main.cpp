using namespace std;

#include "setup.h"
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(10);
#define MQTT_MAX_PACKET_SIZE 512

/******************************** OTHER GLOBALS *******************************/
long state = -1;
float diffTEMP = 0.2;
float tempValue;
float diffHUM = 1;
float humValue;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(SENSOR_PIN, DHT_TYPE);

void setup_wifi()
{
  delay(10);
  Serial.print("Connecting to SSID: ");
  Serial.println(WIFI_SSID);

  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.hostname(DEVICE_NAME);

  if (WiFi.status() != WL_CONNECTED)
  { // FIX FOR USING 2.3.0 CORE (only .begin if not connected)
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/*
 * Calculate Heat Index value AKA "Real Feel"
 * NOAA heat index calculations taken from
 * http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
 */
float calculateHeatIndex(float humidity, float temp)
{
  float heatIndex = 0;
  if (temp >= 80)
  {
    heatIndex = -42.379 + 2.04901523 * temp + 10.14333127 * humidity;
    heatIndex = heatIndex - .22475541 * temp * humidity - .00683783 * temp * temp;
    heatIndex = heatIndex - .05481717 * humidity * humidity + .00122874 * temp * temp * humidity;
    heatIndex = heatIndex + .00085282 * temp * humidity * humidity - .00000199 * temp * temp * humidity * humidity;
  }
  else
  {
    heatIndex = 0.5 * (temp + 61.0 + ((temp - 68.0) * 1.2) + (humidity * 0.094));
  }

  if (humidity < 13 && 80 <= temp <= 112)
  {
    float adjustment = ((13 - humidity) / 4) * sqrt((17 - abs(temp - 95.)) / 17);
    heatIndex = heatIndex - adjustment;
  }

  return heatIndex;
}

void sendState()
{
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject &root = jsonBuffer.createObject();

  root["temperature"] = (String)tempValue;
  root["heatIndex"] = (String)calculateHeatIndex(humValue, tempValue);
  root["humidity"] = (String)humValue;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.println(buffer);
  char combinedArray[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(DEVICE_NAME)];
  sprintf(combinedArray, "%s%s", MQTT_STATE_TOPIC_PREFIX, DEVICE_NAME); // with word space
  client.publish(combinedArray, buffer, true);
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(DEVICE_NAME, MQTT_USER, MQTT_PASSWORD))
    {
      Serial.println("connected");

      char combinedArray[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(DEVICE_NAME) + 4];
      sprintf(combinedArray, "%s%s/set", MQTT_STATE_TOPIC_PREFIX, DEVICE_NAME); // with word space
      client.subscribe(combinedArray);

      sendState();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool checkBoundSensor(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

void updateState()
{
    float newTempValue = dht.readTemperature(); //to use celsius remove the true text inside the parentheses  
    float newHumValue = dht.readHumidity();
    delay(100);
    if (checkBoundSensor(newTempValue, tempValue, diffTEMP)) {
      tempValue = newTempValue;
      sendState();
    }
    if (checkBoundSensor(newHumValue, humValue, diffHUM)) {
      humValue = newHumValue;
      sendState();
    }
}

void setup()
{
  Serial.begin(9600);

  setup_wifi();

  client.setServer(MQTT_SERVER, MQTT_PORT);

  //OTA SETUP
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setHostname(DEVICE_NAME);                // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setPassword((const char *)OTA_PASSWORD); // No authentication by default

  ArduinoOTA.onStart([]() { Serial.println("Starting"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print("Error[%u]: " + error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("Ready");

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output (So it doesnt float as a LED is on this pin)
  pinMode(SENSOR_PIN, INPUT);   // Declare sensor as input
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    delay(1);
    Serial.print("WIFI Disconnected. Attempting reconnection.");
    setup_wifi();
    return;
  }
  client.loop();       // Check MQTT
  ArduinoOTA.handle(); // Check OTA Firmware Updates

  updateState();
}
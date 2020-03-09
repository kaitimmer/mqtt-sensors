#include <ESP8266WiFi.h>

#include <Adafruit_BMP280.h>
#include <DHTesp.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"

WiFiClientSecure wifiClientSecure;
PubSubClient client(wifiClientSecure);

const char *mqttTopic = "sensors";
char sendBuf[128];

DHTesp dht;

Adafruit_BMP280 bmp; // I2C

#define SEALEVELPRESSURE_HPA 1013.25

void setup()
{
  Serial.begin(74880);
  Serial.println("Started..");
  
  // Accept all Certificates on the MQTT Broker
  wifiClientSecure.setInsecure();
  wifiConnect();

  // connect to mqtt server
  client.setServer(mqttServer, mqttPort);
  mqttReconnect();

  dht.setup(14, DHTesp::DHT22);

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,    /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,    /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,   /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,     /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  if (!bmp.begin(0x76))
  {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
  }
}

void wifiConnect()
{
  int tries = 1;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPass);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to WiFi (");
    Serial.print(tries);
    Serial.println("/60)");
    tries += 1;

    // tried for 60s. Go back to sleep
    if (tries >= 60)
    {
      deepSleep();
    }
  }


  Serial.println("Connected to the WiFi network");
  // Print the IP address
  Serial.println(WiFi.localIP());
}

void mqttReconnect()
{
  int tries = 1;

  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection (");
    Serial.print(tries);
    Serial.println("/10)");
    
    // tried for 10s. Go back to sleep
    if (tries >= 10)
    {
      deepSleep();
    }

    // Attempt to connect
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2.5 seconds");
      delay(1000);
    }
    
    tries += 1;
  }
}

void deepSleep(){
  Serial.println("entering deep sleep");
  // deepsleep takes microseconds as an argument (1/1000000) of a second
  // The ESP timer is not 100% acurate so expect some differences

  ESP.deepSleep(interval * 1000000);
  delay(5000);
}

void sendBMP280() {
  sprintf(sendBuf, "data,sensor=BMP280,source=%s pressure=%.2f", clientId, bmp.readPressure() / 100.0F);

  client.publish(mqttTopic, sendBuf);

  Serial.print("Published to MQTT Broker: ");
  Serial.println(sendBuf);
}

void sendDHT22()
{
  delay(dht.getMinimumSamplingPeriod());

  TempAndHumidity measurement = dht.getTempAndHumidity();

  sprintf(sendBuf, "data,sensor=DHT22,source=%s temperature=%.2f,humidity=%.2f", clientId, measurement.temperature, measurement.humidity);

  client.publish(mqttTopic, sendBuf);

  Serial.print("Published to MQTT Broker: ");
  Serial.println(sendBuf);
}

// Reads the brightness from an analog input
// Not converted to lux
void sendBrightness() {
  int sensorValue;
  sensorValue = analogRead(A0);   // read analog input pin 0

  sprintf(sendBuf, "data,sensor=LDR,source=%s value=%i", clientId, sensorValue);

  client.publish(mqttTopic, sendBuf);

  Serial.print("Published to MQTT Broker: ");
  Serial.println(sendBuf);
}

void loop()
{
  client.loop();

  if (!client.connected())
  {
    mqttReconnect();
  }
  
  // send temperature and humidity
  sendDHT22();

  // send air pressure measurement
  sendBMP280();

  // sending brightness (analog voltage read)
  sendBrightness();

  deepSleep();
}

#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"

DHTesp dht;
WiFiClientSecure wifiClientSecure;
PubSubClient client(wifiClientSecure);

const char *mqttTopic = "sensors";

void setup()
{
  Serial.begin(74880);
  Serial.println("Started..");

  dht.setup(14, DHTesp::DHT22);
  
  // Accept all Certificates on the MQTT Broker
  wifiClientSecure.setInsecure();
  wifiConnect();

  // connect to mqtt server
  client.setServer(mqttServer, mqttPort);
  mqttReconnect();
}

void wifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPass);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  // Print the IP address
  Serial.println(WiFi.localIP());
}

void mqttReconnect()
{
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2.5 seconds");
      delay(2500);
    }
  }
}

void loop()
{
  char sendBuf[128];
  
  client.loop();

  if (!client.connected())
  {
    mqttReconnect();
  }

  delay(dht.getMinimumSamplingPeriod());
  
  TempAndHumidity measurement = dht.getTempAndHumidity();
 
  Serial.print("Temperature: ");
  Serial.println(measurement.temperature);
 
  Serial.print("Humidity: ");
  Serial.println(measurement.humidity);

  sprintf(sendBuf, "data,sensor=DHT22,source=%s temperature=%.2f,humidity=%.2f", clientId, measurement.temperature, measurement.humidity);

  client.publish(mqttTopic, sendBuf);
  
  Serial.print("Published to MQTT Broker: ");
  Serial.println(sendBuf);

  delay(interval);
}

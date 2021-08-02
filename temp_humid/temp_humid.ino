#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"

DHTesp dht;
WiFiClientSecure wifiClientSecure;
PubSubClient client(wifiClientSecure);

const char *mqttTopic = "sensors";
char sendBuf[128];

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
  if (deepSleepToggle == 1) {
    Serial.println("entering deep sleep");
    // deepsleep takes microseconds as an argument (1/1000000) of a second
    // The ESP timer is not 100% acurate so expect some differences

    ESP.deepSleep(interval * 1000000);
    delay(100);
  } else {
    Serial.println("deep sleep deactivted,... just waiting");
    delay(interval * 1000);
  }
}

void sendDHT22() {
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
}

void loop()
{
  client.loop();

  if (!client.connected())
  {
    mqttReconnect();
  }
  // send temperature and humidity from DHT22
  sendDHT22();

  deepSleep();
}

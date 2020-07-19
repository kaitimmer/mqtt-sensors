#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"

WiFiClientSecure wifiClientSecure;
PubSubClient client(wifiClientSecure);

const char *mqttTopic = "sensors";
char sendBuf[128];

// Define Trig and Echo pin:
#define trigPin 15
#define echoPin 13

// Define variables:


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

  // Define inputs and outputs
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
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
      delay(interval * 1000);
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
      delay(interval * 1000);
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

void sendCistern() {
  long duration;
  float level;
  float distance;

  digitalWrite(trigPin, LOW);
  
  delayMicroseconds(5);
  // Trigger the sensor by setting the trigPin high for 10 microseconds:
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);
  // Read the echoPin. pulseIn() returns the duration (length of the pulse) in microseconds:
  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance:
  distance = duration * 0.034 / 2.0;

  // Calculate filling in percentage (approx)
  level = (-100.0/157.0) * (distance - 13.0) + 100.0;

  sprintf(sendBuf, "data,sensor=JSN-SR04T-2.0,source=%s level=%.1f,distance=%.2f", clientId, level, distance);

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
  // send cistern filling level
  sendCistern();

  delay(interval * 1000);
  //delay(1000);
}

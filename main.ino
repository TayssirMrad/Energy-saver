 /*
/ **

@file ESP32_code
* @brief Hotel Energy Saver - ESP32 Firmware

* This firmware monitors and optimizes energy usage in hotel environments
* using ESP32 microcontroller. It collects data from various sensors,
* implements energy-saving algorithms, and communicates with a Node-RED
* dashboard via MQTT.
*

* @TayssirMrad
* @ 2023
* @version 1.0
*/
  
#include <WiFi.h>
#include "DHT.h"
#include <PubSubClient.h>

//Wi-Fi credentials
const char* ssid = "MY-WIFI";
const char* password = "MYPSWD";

//MQTT Broker details
const char* mqtt_server ="10.192.96.152";//RaspberryPi IP
const int mqtt_port = 1883;
const char* mqtt_user ="";
const char* mqtt_password ="";

//MQTT topics
const char* temperature_topic ="room/temperature";
const char* humidity_topic ="room/humidity";
const char* status_topic = "esp32/dht22/status";


//Sensor setup
#define DHTTYPE DHT22 
#define DHTPIN 25
DHT dht(DHTPIN, DHTTYPE);

// client objects
WiFiClient espclient;
PubSubClient client(espclient);

// variables
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50);


void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

}

void setup_wifi(){
    // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  }

  void callback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming messages if needed
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("ESP32DHTClient", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(status_topic, "ESP32-DHT22 connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
 if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(1000);
    float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // check if any reads failed
  if (isnan(h)|| isnan(t)){
    Serial.println("Failed to read from sensor!");
    client.publish(status_topic, "sensor error");
  return;
  }

//Publish to NodeRED
    // Convert temperature to string
    char tempString[8];
    dtostrf(t, 1, 2, tempString);
    client.publish(temperature_topic, tempString);
    
    // Convert humidity to string
    char humString[8];
    dtostrf(h, 1, 2, humString);
    client.publish(humidity_topic, humString);


  
  Serial.print("\nHumidity : ");
  Serial.print(h);
  Serial.print("% Temperature : ");
  Serial.print(t);
  Serial.print("°C ");



  
}

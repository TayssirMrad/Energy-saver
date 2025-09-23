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


// MQTT topics
const char* status_topic = "esp32/dht22/status";

// Set this per room
const char* room_number = "101";

// Build topics dynamically
char base_topic[50];
char light_value_topic[60];
char light_state_topic[60];
char heating_value_topic[60];
char heating_state_topic[60];
char temperature_topic[60];
char humidity_topic[60];

// Sensor setup
#define DHTTYPE DHT22 
#define DHTPIN 25
DHT dht(DHTPIN, DHTTYPE);

// Client objects
WiFiClient espClient;
PubSubClient client(espClient);

// Variables
unsigned long last_update = 0;
uint8_t light_status = 1;
uint8_t heater_status = 1;
float light_energy = 0.0; // kWh
float heater_energy = 0.0; // kWh
const float light_power = 9.0; // watts
const float heater_power = 1000.0; // watts
float light_power_consumption = 0.0;
float heater_power_consumption = 0.0;

// Function prototypes
void setup_wifi(void);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect(void);
void calculate_consumption(void);
void build_topics(void);

void setup() {
    Serial.begin(115200);
    dht.begin();
    
    // Build MQTT topics
    build_topics();
    
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    last_update = millis(); // Initialize last_update
}

void build_topics(void) {
   /*
/ **
Topics :
base_topic : hotel/101
light_value_topic : hotel/101/light/value
light_state_topic : hotel/101/light/state
heating_value_topic : hotel/101/heating/value
heating_state_topic : hotel/101/heating/state
temperature_topic : hotel/101/temperature
humidity_topic : hotel/101/humidity
*/
    snprintf(base_topic, sizeof(base_topic), "hotel/%s", room_number);
    snprintf(light_value_topic, sizeof(light_value_topic), "%s/light/value", base_topic);
    snprintf(light_state_topic, sizeof(light_state_topic), "%s/light/state", base_topic);
    snprintf(heating_value_topic, sizeof(heating_value_topic), "%s/heating/value", base_topic);
    snprintf(heating_state_topic, sizeof(heating_state_topic), "%s/heating/state", base_topic);
    snprintf(temperature_topic, sizeof(temperature_topic), "%s/temperature", base_topic);
    snprintf(humidity_topic, sizeof(humidity_topic), "%s/humidity", base_topic);
}

void setup_wifi(void) {
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

void reconnect(void) {
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
            delay(5000);
        }
    }
}

void calculate_light_energy(void) {
      /*
    Energy : E[KW/h] = P[KW] x t[h] 
    Heater : Ph = 1.0kW
    Light : Pl = 9w = 0.009kW
    */  
    unsigned long current_time = millis(); // ms
    unsigned long elapsed_time = current_time - last_update; 
    float time_hours = elapsed_time / 3600000.0; // ms to hours

    if (light_status) {
        light_energy += (light_power * time_hours) / 1000.0; // kWh  
        light_power_consumption = light_power;
    } else {
        light_power_consumption = 0.0;
    }
}

void calculate_heater_energy(void) {
      /*
    Energy : E[KW/h] = P[KW] x t[h] 
    Heater : Ph = 1.0kW
    Light : Pl = 9w = 0.009kW
    */  
    unsigned long current_time = millis(); // ms
    unsigned long elapsed_time = current_time - last_update; 
    float time_hours = elapsed_time / 3600000.0; // ms to hours

    if (heater_status) {
        heater_energy += (heater_power * time_hours) / 1000.0; // kWh  
        heater_power_consumption = heater_power;
    } else {
        heater_power_consumption = 0.0;
    }
    
    // Update last_update time for next calculation
    last_update = current_time;
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Calculate consumption first
    calculate_light_energy();
    calculate_heater_energy();


    delay(2000); // Wait 2 seconds between readings
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Check if any reads failed
    if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from sensor!");
        client.publish(status_topic, "sensor error");
        return;
    }

    // Publish sensor data and status
    
    // Light value (energy consumption)
    char light_str[10];
    snprintf(light_str, sizeof(light_str), "%.2f", light_energy);
    client.publish(light_value_topic, light_str);
    
    // Light state
    client.publish(light_state_topic, light_status ? "ON" : "OFF");
    
    // Heating value (energy consumption)
    char heat_str[10];
    snprintf(heat_str, sizeof(heat_str), "%.2f", heater_energy);
    client.publish(heating_value_topic, heat_str);
    
    // Heating state
    client.publish(heating_state_topic, heater_status ? "ON" : "OFF");
    
    // Temperature     
    char temp_str[10];
    snprintf(temp_str, sizeof(temp_str), "%.2f", t);
    client.publish(temperature_topic, temp_str);
    
    // Humidity 
    char hum_str[10];
    snprintf(hum_str, sizeof(hum_str), "%.2f", h);
    client.publish(humidity_topic, hum_str);

    // Print to serial monitor
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print("% Temperature: ");
    Serial.print(t);
    Serial.print("°C Light consumption: ");
     Serial.print(light_power_consumption);
    Serial.println("W Heater consumption: ");
    Serial.print(heater_power_consumption);
    Serial.println("W");
}

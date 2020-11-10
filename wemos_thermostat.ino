#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHTesp.h>
#include <ESP8266WebServer.h>
#include "WiFiPassword.h"


// v0
//#define BUS_PIN 5
//#define RELAY_PIN 16

// v1
// #define BUS_PIN 13 // D7 on wemos
// #define RELAY_PIN 5 // D1 on wemos
// #define DHT22_PIN 2 //D4 on wemos

#define DHT22_ONBOARD_PIN 5 //D4 on wemos

// v2
#define BUS_PIN 14 // D5 on wemos
#define RELAY_PIN 12 // D6 on wemos
//#define DHT22_PIN 15 //D8 on wemos

// v3
#define DHT22_PIN DHT22_ONBOARD_PIN


#define METRIC_PREFIX "debug_"

ESP8266WebServer server(80);

uint8_t nSensors = 0;
OneWire Bus(BUS_PIN);
DallasTemperature Sensors(&Bus);
boolean heatingOn = false;
float targetTemperature = 5;

DHTesp dht;

void handleRoot();
void handleHeatingOn();
void handleHeatingOff();
void setTargetTemperature();
void getTemp();
void updateTemperatures();

/**/
float dhtHumidity;
float dhtTemperature;
float sensorTemperatures[10]; // no more than 10 sensors will be reported
unsigned long lastUpdatedTime = 0;



void setup() {

  Serial.begin(115200);
  //pinMode(BUS_PIN, INPUT);
  Sensors.begin();
  dht.setup(DHT22_PIN, DHTesp::DHT22);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  //digitalWrite(RELAY_PIN, LOW);


  delay(10);

  // Connect WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.hostname("thermostat");
  #ifdef STATIC_IP
  WiFi.config(staticIP, gateway, subnet, dns);
  WiFi.mode(WIFI_STA);
  #endif
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  nSensors = Sensors.getDeviceCount();
  Serial.print("Number of Sensors: ");
  Serial.println(nSensors);

  updateTemperatures();

  server.on("/", handleRoot);
  server.on("/heating/on", handleHeatingOn);
  server.on("/heating/off", handleHeatingOff);
  server.on("/setTargetTemperature", setTargetTemperature);
  server.begin();
}

void loop() {
  server.handleClient();
  updateTemperatures();
}

void updateTemperatures() {
  if ((millis() - lastUpdatedTime) < 2000) return;
  Serial.println("Updating Temperature data");

  dhtHumidity = dht.getHumidity();
  dhtTemperature = dht.getTemperature();

  Sensors.requestTemperatures();

  for (uint8_t i = 0; i < nSensors; i++)
  {
    sensorTemperatures[i] = Sensors.getTempCByIndex(i);
  }
  lastUpdatedTime = millis();
}

String prepareMetricsPage()
{
  Serial.println("prepareMetricsPage()");
  String temps = String();
  long rssi = WiFi.RSSI();

  for (uint8_t i = 0; i < nSensors; i++)
  {
    String name = METRIC_PREFIX + String("outside_house_temperature_") + String(i);
    temps = temps + "# TYPE " + name + " gauge\n";
    temps =  temps + name + " " + String(sensorTemperatures[i]) + "\n";
  }

  temps = temps + "# TYPE " + METRIC_PREFIX + "outside_house_wifi_signal_strength gauge\n";
  temps =  temps + METRIC_PREFIX + "outside_house_wifi_signal_strength " + String(rssi) + "\n";

  temps = temps + "# TYPE " + METRIC_PREFIX + "outside_house_humidity gauge\n";
  temps =  temps + METRIC_PREFIX + "outside_house_humidity " + String(dhtHumidity) + "\n";

  temps = temps + "# TYPE " + METRIC_PREFIX + "outside_house_device_temperature gauge\n";
  temps =  temps + METRIC_PREFIX + "outside_house_device_temperature " + String(dhtTemperature) + "\n";


  temps = temps + "# TYPE " + METRIC_PREFIX + "outside_house_heating_status gauge\n";
  temps =  temps + METRIC_PREFIX + "outside_house_heating_status " + String(heatingOn ? 1 : 0) + "\n";

  temps = temps + "# TYPE " + METRIC_PREFIX + "outside_house_target_temperature gauge\n";
  temps =  temps + METRIC_PREFIX + "outside_house_target_temperature " + String(targetTemperature) + "\n";

  return temps;
}

void handleHeatingOn() {
    digitalWrite(RELAY_PIN, LOW);
    heatingOn = true;
    server.send(200, "text/plain", "ON");
}

void handleHeatingOff() {
    digitalWrite(RELAY_PIN, HIGH);
    heatingOn = false;
    server.send(200, "text/plain", "OFF");
}
void setTargetTemperature() {
    targetTemperature = server.arg("t").toFloat();
    server.send(200, "text/plain", "targetTemperature " + String(targetTemperature));
}


void handleRoot() {
    Serial.println("Handling /");
    server.send(200, "text/plain", prepareMetricsPage());
}
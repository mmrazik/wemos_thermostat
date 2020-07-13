#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHTesp.h>
#include <ESP8266WebServer.h>
#include "WiFiPassword.h"


//stara verze
//#define BUS_PIN 5
//#define RELAY_PIN 16

#define BUS_PIN 13 // D7 on wemos
#define RELAY_PIN 5 // D1 on wemos
#define DHT22_PIN 2 //D4 on wemos

ESP8266WebServer server(80);

uint8_t nSensors = 0;
OneWire Bus(BUS_PIN);
DallasTemperature Sensors(&Bus);
DeviceAddress DevAdr;
boolean heatingOn = false;
float targetTemperature = 5;

DHTesp dht;

void handleRoot();
void handleHeatingOn();
void handleHeatingOff();
void setTargetTemperature();
void getTemp();


void setup() {

  Serial.begin(115200);
  pinMode(BUS_PIN, INPUT);
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

  server.on("/", handleRoot);
  server.on("/heating/on", handleHeatingOn);
  server.on("/heating/off", handleHeatingOff);
  server.on("/setTargetTemperature", setTargetTemperature);
  server.begin();
  delay(2000);
}

void loop() {
  server.handleClient();
  // getTemp();
}


void getTemp(){
  Sensors.requestTemperatures();

  for (uint8_t i = 0; i < nSensors; i++)
  {
    Serial.print("Sensor temperature #");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(Sensors.getTempCByIndex(i));
  }
  Serial.println("****************************");
  delay(2000);
}


String prepareMetricsPage()
{
  Sensors.requestTemperatures();

  String temps = String();
  long rssi = WiFi.RSSI();

  float hum = dht.getHumidity();
  float device_temp = dht.getTemperature();

  for (uint8_t i = 0; i < nSensors; i++)
  {
    String name = String("outside_house_temperature_") + String(i);
    temps = temps + "# TYPE " + name + " gauge\n";
    temps =  temps + name + " " + String(Sensors.getTempCByIndex(i)) + "\n";
  }

  temps = temps + "# TYPE outside_house_wifi_signal_strength gauge\n";
  temps =  temps + "outside_house_wifi_signal_strength " + String(rssi) + "\n";

  temps = temps + "# TYPE outside_house_humidity gauge\n";
  temps =  temps + "outside_house_humidity " + String(hum) + "\n";

  temps = temps + "# TYPE outside_house_device_temperature gauge\n";
  temps =  temps + "outside_house_device_temperature " + String(device_temp) + "\n";

  temps = temps + "# TYPE outside_house_heating_status gauge\n";
  temps =  temps + "outside_house_heating_status " + String(heatingOn ? 1 : 0) + "\n";

  temps = temps + "# TYPE outside_house_target_temperature gauge\n";
  temps =  temps + "outside_house_target_temperature " + String(targetTemperature) + "\n";

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
    server.send(200, "text/plain", prepareMetricsPage());
    delay(1200);
}
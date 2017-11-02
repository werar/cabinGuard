/**************************************************************
* TODO time to time do init_GSM
 *
 **************************************************************/
 #include <Wire.h>                                                       // required by BME280 library
 #include <BME280_t.h>

 #include "main.h"

 BME280<> BMESensor;
 parameters_type parameters;

// Select your modem:
//#define TINY_GSM_MODEM_SIM800
// #define TINY_GSM_MODEM_SIM808
// #define TINY_GSM_MODEM_SIM900
// #define TINY_GSM_MODEM_A6
 #define TINY_GSM_MODEM_A7
// #define TINY_GSM_MODEM_M590
// #define TINY_GSM_MODEM_ESP8266
// #define TINY_GSM_MODEM_XBEE

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "internet";
const char user[] = "internet";
const char pass[] = "interent";

// Use Hardware Serial on Mega, Leonardo, Micro
//#define SerialAT Serial1

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3); // RX, TX

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

const char* broker = "vps.weraksa.net";

const char* topicLed = "Altana/sensor1/led";
const char* topicLog = "Altana/sensor1/log";
const char* topicLedStatus = "Altana/sensor1/ledStatus";
const char* topicTemp = "Altana/sensor1/temp";
const char* topicHum = "Altana/sensor1/hum";
const char* topicPressure = "Altana/sensor1/pressure";

#define LED_PIN 13
int ledStatus = LOW;

long lastReconnectAttempt = 0;

void print_BMEData() //TODO: save that to some structure.
{
  BMESensor.refresh();                                                  // read current sensor data

  Serial.print("Temperature: ");
  Serial.print(BMESensor.temperature);                                  // display temperature in Celsius
  Serial.println("C");

  Serial.print("Humidity:    ");
  Serial.print(BMESensor.humidity);                                     // display humidity in %
  Serial.println("%");

  Serial.print("Pressure:    ");
  Serial.print(BMESensor.pressure  / 100.0F);                           // display pressure in hPa
  Serial.println("hPa");
}

void get_BMEData()
{
  BMESensor.refresh();                                                  // read current sensor data
  parameters.temperature=BMESensor.temperature;  //TOD: use long not int
  parameters.humidity=BMESensor.humidity;                                     // display humidity in %
  parameters.pressure=BMESensor.pressure  / 100.0F;                           // display pressure in hPa
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicLed) {
    ledStatus = !ledStatus;
    digitalWrite(LED_PIN, ledStatus);
    mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");
  }
}

bool init_GPRS()
{
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);

  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    while (true);
  }
  Serial.println(" OK");

  Serial.print("Connecting to ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    while (true);
  }
  Serial.println(" OK");
  return true;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);

  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(28800);
  delay(3000);

  Wire.begin();
  BMESensor.begin();



init_GPRS();
  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}

boolean mqttConnect() {
  Serial.print("Connecting to ");
  Serial.print(broker);
  if (!mqtt.connect("GsmClientTest","user1","welcome1")) {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" OK");
  mqtt.publish(topicLog, "Sensor1 started");
  mqtt.subscribe(topicLed);
  return mqtt.connected();
}
unsigned long last_time_to_send_report = 0;

void loop() {
  unsigned long time_to_send_report = millis();
  if (mqtt.connected()) {
    mqtt.loop();
    if(time_to_send_report-last_time_to_send_report>10000L)
    {
      get_BMEData();
      print_BMEData();
      char str[15];
      sprintf(str, "%.1f  ", (double)parameters.temperature);
      mqtt.publish(topicTemp, str);
      sprintf(str, "%d  ", parameters.humidity);
      mqtt.publish(topicHum, str);
      sprintf(str, "%.1f  ", (double)parameters.pressure);
      mqtt.publish(topicPressure, str);

      last_time_to_send_report=time_to_send_report;
    }
  } else {
    // Reconnect every 10 seconds
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }

}

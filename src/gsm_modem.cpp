
/***
* TODO: check if someone is
* TODO: make uniwersal way to catch serial

https://dweet.io/faq
https://www.freeboard.io/board/edit/rBbZQR

*/

#include <Arduino.h>
#include <PubSubClient.h>
#include "gsm_modem.h"
#include "main.h"

#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3); // RX, TX
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

//#include <StreamDebugger.h>
//StreamDebugger StreamDbg(SerialAT, Serial);

const char* broker = "176.122.224.123";//"vps.weraksa.net";
const char* topicLed = "GsmClientTest/led";
const char* topicInit = "GsmClientTest/init";
const char* topicLedStatus = "GsmClientTest/ledStatus";

//#define DEBUG //uncomment that if you want to see messages on Serial console
/***
* M590 chip MUST to be supplied near 4.2V! 3.3 is not enough!
*/
void init_GSM()
{
  Serial.print("Initializing modem...");
  //TinyGsmAutoBaud(SerialAT);
  SerialAT.begin(28800);
  delay(5000);

  if (!modem.restart())
  {
    Serial.println("modem restart failed");
    delay(10000);
    return;
  }
    Serial.println("done");
    Serial.println(modem.getModemInfo());
    delay(10000);
}


bool send_sms(const char* message, const char* phone_no)
{
  return modem.sendSMS(phone_no, message);
}

/***
* If you set CLIP=1 during the call the caller number will be shown. Below dump of the modem output:
AT+CLIP=1
OK
RING
+CLIP: "517083663",129,,,"",0
RING
+CLIP: "517083663",129,,,"",0
*
*TODO: migrate to pure avr soluiton
*/
bool is_calling(char* caller_number)
{
  char buffer[100];//="OK\n\RING\n+CLIP: \"517083663\",129,,,\"\",0\nRING\+CLIP: \"517083663\",129,,,\"\",0";
  if (SerialAT.available() > 0)
  {
    SerialAT.readBytesUntil('\r', buffer, 100);
    char * pch;
    pch = strstr(buffer,"+CLIP");
    if(pch!=NULL)
    {
      pch = strstr(buffer,caller_number);
      if(pch!=NULL)
      { //action to disable/enable alerting
        PORTB ^= (1 << PB5);
        return true;
      }else{
        return false;
      }
    }
  }
  return false;
}

/***
*
https://forum.arduino.cc/index.php?topic=486845.0
http://www.arturnet.pl/modem-m590-obsluga-arduino-wywolujemy-restapi/
V
*/
bool send_telemetry_report(const char* report)
{

  if (!modem.waitForNetwork())
  {
    delay(10000);
    return false;
  }
    if (!modem.gprsConnect("internet", "internet", "internet")) {
      delay(10000);
      return false;
    }
    if (!client.connect("34.203.32.119", 80)) {
      delay(10000);
      return false;
    }
    client.print(report);
    client.stop();
    modem.gprsDisconnect();
    return true;
}

/***
*
https://github.com/vshymanskyy/TinyGSM/blob/master/examples/MqttClient/MqttClient.ino
*/
bool send_telemetry_report_to_mqtt(parameters_type* parameters)
{
    Serial.println("Sending telemetry report via mqtt");
      Serial.println("Waiting for network...");
    if (!modem.waitForNetwork()) {
      delay(10000);
      return false;
    }
    Serial.println("Odpalam gprs");
    if (!modem.gprsConnect("internet", "internet", "internet")) {
      delay(10000);
      Serial.println("GPRS nie odpalil");
      return false;
    }

    // MQTT Broker setup
    Serial.println("Odpalam mqtt");
    mqtt.setServer(broker, 1883);
    mqtt.setCallback(mqttCallback);
    mqttConnect();
    mqtt.publish(topicInit, "Test");
    Serial.println("Zamykam gprs");
    modem.gprsDisconnect();
    return true;
}

bool mqttConnect() {
  Serial.print("Connecting to ");
  Serial.print(broker);
  if (!mqtt.connect("GsmClientTest","user1","welcome1")) {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" OK");
  mqtt.publish(topicInit, "GsmClientTest started");
  mqtt.subscribe(topicLed);
  return mqtt.connected();
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicLed) {
    //ledStatus = !ledStatus;
    //digitalWrite(LED_PIN, ledStatus);
    //mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");
  }
}

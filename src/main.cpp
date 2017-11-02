/***
*
*
* TODO: fix function names convention based on that? http://www.ganssle.com/misc/fsm.doc
* TODO: sms is full - add someting checking that
* https://shortn0tes.blogspot.com/2016/05/neoway-m590-gprs-tutorial-sending-and.html (look at comments)
* TODO: use interesting String class: http://arduiniana.org/libraries/pstring/
*/

#include <Arduino.h>
#include <Wire.h>                                                       // required by BME280 library
#include <BME280_t.h>
#include <SPI.h>
#include <U8g2lib.h>

#include "main.h"
//#include "gsm_modem.h"

//#define DEBUG

BME280<> BMESensor;

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(5,4);
parameters_type parameters;                                               // instantiate sensor
volatile timers_type timers;

//---------------------------------------------------------------------------------------------------
#define TINY_GSM_MODEM_A7
//#define TINY_GSM_MODEM_M590

#include <TinyGsmClient.h>

#include <PubSubClient.h>
//#include "gsm_modem.h"
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3); // RX, TX
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
const char* broker = "176.122.224.123";//"vps.weraksa.net";
const char* topicLed = "GsmClientTest/led";
const char* topicInit = "GsmClientTest/init";
const char* topicLedStatus = "GsmClientTest/ledStatus";

bool send_sms(const char* message, const char* phone_no)
{
  return modem.sendSMS(phone_no, message);
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

void init_GSM()
{
  Serial.print("Initializing modem...");
  //TinyGsmAutoBaud(SerialAT);
  //SerialAT.begin(28800);
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


//---------------------------------------------------------------------------------------

void print_BMEData() //TODO: save that to some structure.
{
  char bufout[10];
  BMESensor.refresh();                                                  // read current sensor data
  sprintf(bufout,"%c[1;0H",ASCII_ESC);
  Serial.print(bufout);

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

void draw(void)
{
  if(&u8x8==NULL)  return;
  // graphic commands to redraw the complete screen should be placed here
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString( 0, 0, "Telemetry:");
  char str[15];
  sprintf(str, "%.1fC  ", (double)parameters.temperature);
  u8x8.drawString( 0, 1, str);
  sprintf(str, "%d%%  ", parameters.humidity);
  u8x8.drawString( 0, 2, str);
  sprintf(str, "%.1fhPa  ", (double)parameters.pressure);
  u8x8.drawString( 0, 3, str);
  if(parameters.enable_alert)
  {
    u8x8.drawString( 0, 4, "alarm enabled ");
  }else
  {
    u8x8.drawString( 0, 4, "alarm disabled");
  }
  if(parameters.pir_alert)
  {
    u8x8.drawString( 0, 5, "pir alert: true ");
  }else
  {
    u8x8.drawString( 0, 5, "pir alert: false");
  }
  sprintf(str, "%d ", timers.sent_telemetry_report);
  u8x8.setCursor(0, 6);
  u8x8.print("               ");
  u8x8.drawString(0, 6, str);
}
/***
* The regsisters values can be calulated here: http://www.arduinoslovakia.eu/application/timer-calculator
*/
void init_avr_timers()
{
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1A |= (1<<WGM12); //CTC mode timer1
  TCCR1B |= (1<<CS11)|(1<<CS10); //prescaler64
  // 1 Hz (80000/((1249+1)*64))
  OCR1A = 1249; //(target time) = (timer resolution) * (# timer counts + 1)
  TIMSK1 |= (1<<OCIE1A);
  sei();
}

ISR(TIMER1_COMPA_vect)
{
  //PORTB ^= (1 << PB5); //bulit in led
  timers.secounds--;
  timers.sent_telemetry_report--;
  timers.reenable_alerts--;
}

void reset_timers()
{
  timers.secounds=0;
  timers.sent_telemetry_report=SECOUNDS_TO_SEND_TELEMETRY_REPORT;
  timers.is_alert_was_sent=false;
  timers.reenable_alerts=SECOUNDS_TO_ARM_ALERTS;
}

void check_movement()
{
  int val = digitalRead(PIR_PIN);   // read the input pin
  if(val==LOW)
  {
    parameters.pir_alert=false;
  }else
  {
    parameters.pir_alert=true;
  }
}


void alerts()
{
  bool disable_alerts=false;
  //disable_alerts=is_calling(PHONE_TO_UNARM);
  if(disable_alerts)
  {
    parameters.enable_alert=false;
  }
  if(timers.reenable_alerts<=0)
  {
    parameters.enable_alert=true;
    timers.is_alert_was_sent=false;
  }
  if(parameters.pir_alert && !timers.is_alert_was_sent && parameters.enable_alert)
  {
    Serial.println("Alert occurs");
    if(!send_sms(ALERT_MESSAGE_TEXT,PHONE_TO_CALL))
    {
      Serial.println("No success. Sending SMS again.");
      init_GSM();
      send_sms(ALERT_MESSAGE_TEXT,PHONE_TO_CALL);
    }else{
      Serial.println("Alert was sent via SMS");
    }
    timers.is_alert_was_sent=true;
    timers.reenable_alerts=SECOUNDS_TO_ARM_ALERTS;
    delay(3000); //workaroud for rf noise during sending packages via GSM modem
  }
}


void messages_and_reports()
{

  if(timers.sent_telemetry_report<=0)
  {
    //char text_to_sent[150]; //"POST /dweet/for/werar1234?test=1 HTTP/1.1\r\nHost: dweet.io\r\nConnection: close\r\nAccept: */*\r\n\r\n"
    //sprintf(text_to_sent, "POST /dweet/for/werar1234?temp=%.2f&hum=%d&press=%.2f&pir_alert=%d HTTP/1.1\r\nHost: dweet.io\r\nConnection: close\r\nAccept: */*\r\n\r\n", (double)parameters.temperature,parameters.humidity,(double)parameters.pressure,parameters.pir_alert);

    init_GSM();
    if(!send_telemetry_report_to_mqtt(&parameters))
    {
      Serial.println("Sending report failed");
    }else
    {
      Serial.println("Sending report success");
    }
    timers.sent_telemetry_report=SECOUNDS_TO_SEND_TELEMETRY_REPORT;
  }
}

void setup()
{
  Serial.begin(115200);                                               // initialize serial
  Serial.println("System is started");
  SerialAT.begin(28800);
  pinMode(PIR_PIN,INPUT_PULLUP);
  Wire.begin();
  u8x8.begin();
  BMESensor.begin();
  init_GSM();
  init_avr_timers();
  reset_timers();
  parameters.enable_alert=true;
  Serial.println("Go to the main loop");
  delay(1000);
}

void loop()
{
   Serial.print("o");
   get_BMEData();
   check_movement();
   messages_and_reports();
   //alerts();
   draw();
   delay(1000);                                                    // wait a while before next loop
}

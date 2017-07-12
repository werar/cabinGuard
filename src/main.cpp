/***
* TODO: temperature as float
* TODO: check who is calling if known number arm/disarm sending alerts
* At+clip=1 or at+clcc during ring
* "AT#CID=1" - to enable caller ID
*
* TODO: use power off to save energy AT+CPWROFF
* TODO: fix function names convention  based on that? http://www.ganssle.com/misc/fsm.doc
* TODO: maybe via GPRS to send reports alerts as well?
* https://shortn0tes.blogspot.com/2016/05/neoway-m590-gprs-tutorial-sending-and.html (look at comments)
*/

#include <Arduino.h>
#include <Wire.h>                                                       // required by BME280 library
#include <BME280_t.h>
#include "main.h"
#include "gsm_modem.h"

#define MYALTITUDE  150.50
#define DEBUG


BME280<> BMESensor;
parameters_type parameters;                                               // instantiate sensor
volatile timers_type timers;

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

  float relativepressure = BMESensor.seaLevelForAltitude(MYALTITUDE);
  Serial.print("RelPress:    ");
  Serial.print(relativepressure  / 100.0F);                             // display relative pressure in hPa for given altitude
  Serial.println("hPa");

  Serial.print("Altitude:    ");
  Serial.print(BMESensor.pressureToAltitude(relativepressure));         // display altitude in m for given pressure
  Serial.println("m");
}

void get_BMEData()
{
  BMESensor.refresh();                                                  // read current sensor data
  parameters.temperature=BMESensor.temperature;  //TOD: use long not int
  parameters.humidity=BMESensor.humidity;                                     // display humidity in %
  parameters.pressure=BMESensor.pressure  / 100.0F;                           // display pressure in hPa
}


/***
* The regsisters values can be calulated here: http://www.arduinoslovakia.eu/application/timer-calculator
*/
void init_timers()
{
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1A |= (1<<WGM12); //CTC mode timer1
  TCCR1B |= (1<<CS11)|(1<<CS10); //prescaler64
  // 1 Hz (80000/((1249+1)*64))
  OCR1A = 1249; //(target time) = (timer resolution) * (# timer counts + 1)  //TODO: not sure if that is 1 sec
  TIMSK1 |= (1<<OCIE1A);
  sei();
}

ISR(TIMER1_COMPA_vect)
{
  PORTB ^= (1 << PB5); //bulit in led
  timers.secounds--;
  timers.sent_message--;
  timers.sent_telemetry_report--;
}

void reset_timers()
{
  timers.secounds=0;
  timers.sent_telemetry_report=SECOUNDS_TO_SEND_TELEMETRY_REPORT;
  timers.sent_message=SECOUNDS_TO_WAIT_WITH_ALERT_MESSAGE;
  timers.is_alert_was_sent=false;
}

void check_movement()
{
  int val = digitalRead(PIR_PIN);   // read the input pin
  if(val==0)
  {
    parameters.pir_alert=false;
    return;
  }
  parameters.pir_alert=true;
  return;
}

void messages_and_reports()
{
  if(parameters.pir_alert && !timers.is_alert_was_sent)
  {
    send_sms(ALERT_MESSAGE_TEXT,PHONE_TO_CALL);
    timers.is_alert_was_sent=true;
    timers.sent_message=SECOUNDS_TO_WAIT_WITH_ALERT_MESSAGE;
  }else
  {
    if(timers.sent_message<=0)
    {
      timers.is_alert_was_sent=false;
    }
  }

  if(timers.sent_telemetry_report<=0)
  {
    char text_to_sent[248];
    strcpy(text_to_sent,"Telemetry report\n");
    char buf[20];
    //sprintf(buf, "Temperature: %.1f\n", (double)parameters.temperature);
    sprintf(buf, "Temperature: %d\n",parameters.temperature);
    strcat(text_to_sent,buf);
    sprintf(buf, "Humidity: %d%%\n", parameters.humidity);
    strcat(text_to_sent,buf);
    sprintf(buf, "Pressure: %d\n", parameters.pressure);
    strcat(text_to_sent,buf);
    send_sms(text_to_sent,PHONE_TO_CALL);
    timers.sent_telemetry_report=SECOUNDS_TO_SEND_TELEMETRY_REPORT;
  }
}

void setup()
{
  Serial.begin(4800);                                                 // initialize serial
  pinMode(PIR_PIN, INPUT);  //TODO: use avr convention
  init_GSM();
  delay(5000);
  BMESensor.begin();                                                    // initalize bme280 sensor
  init_timers();
  reset_timers();
}

void loop()
{
   get_BMEData();
   check_movement();
   messages_and_reports();
   delay(100);                                                          // wait a while before next loop
}

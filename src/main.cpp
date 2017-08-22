/***
*
*
* TODO: fix function names convention based on that? http://www.ganssle.com/misc/fsm.doc
* TODO: sms is full - add someting checking that
* https://shortn0tes.blogspot.com/2016/05/neoway-m590-gprs-tutorial-sending-and.html (look at comments)
*/

#include <Arduino.h>
#include <Wire.h>                                                       // required by BME280 library
#include <BME280_t.h>
#include <SPI.h>
#include <U8g2lib.h>

#include "main.h"
#include "gsm_modem.h"

//#define DEBUG

BME280<> BMESensor;

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(5,4);
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
  if(val==0)
  {
    parameters.pir_alert=false;
    return;
  }
  parameters.pir_alert=true;
  return;
}

void alerts()
{
  bool disable_alerts=false;
  disable_alerts=is_calling(PHONE_TO_UNARM,u8x8);
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
    send_sms(ALERT_MESSAGE_TEXT,PHONE_TO_CALL);
    timers.is_alert_was_sent=true;
    timers.reenable_alerts=SECOUNDS_TO_ARM_ALERTS;
  }
}

void messages_and_reports()
{
  draw();
  if(timers.sent_telemetry_report<=0)
  {
    char text_to_sent[150]; //"POST /dweet/for/werar1234?test=1 HTTP/1.1\r\nHost: dweet.io\r\nConnection: close\r\nAccept: */*\r\n\r\n"
    sprintf(text_to_sent, "POST /dweet/for/werar1234?temp=%.2f&hum=%d&press=%.2f&pir_alert=%d HTTP/1.1\r\nHost: dweet.io\r\nConnection: close\r\nAccept: */*\r\n\r\n", (double)parameters.temperature,parameters.humidity,(double)parameters.pressure,parameters.pir_alert);
    #ifdef DEBUG
    Serial.println(text_to_sent);
    #else
    send_telemetry_report(text_to_sent); //BUG: the soft hangs after sending few messages
    #endif
    timers.sent_telemetry_report=SECOUNDS_TO_SEND_TELEMETRY_REPORT;
  }
}

void messages_and_reports_via_sms()
{
  draw();
  if(timers.sent_telemetry_report<=0)
  {
    char text_to_sent[100];
    strcpy(text_to_sent,"Telemetry report\n");
    char buf[20];
    sprintf(buf, "Temperature: %.2f\n", (double)parameters.temperature);
    strcat(text_to_sent,buf);
    sprintf(buf, "Humidity: %d%%\n", parameters.humidity);
    strcat(text_to_sent,buf);
    sprintf(buf, "Pressure: %.2fhPa\n", (double)parameters.pressure);
    strcat(text_to_sent,buf);
    send_sms(text_to_sent,PHONE_TO_CALL);
    timers.sent_telemetry_report=SECOUNDS_TO_SEND_TELEMETRY_REPORT;
  }
}

void setup()
{
  Serial.begin(4800);                                                 // initialize serial
  pinMode(PIR_PIN, INPUT);  //TODO: use avr convention
  u8x8.begin();
  delay(8000); //TODO: check if modem is ready (+PBREADY)
  init_GSM();
  init_GPRS();
  BMESensor.begin();                                                    // initalize bme280 sensor
  init_avr_timers();
  reset_timers();
  parameters.enable_alert=true;
}

void loop()
{
   get_BMEData();
   //print_BMEData();
   check_movement();
   messages_and_reports();
   alerts();
   delay(10);                                                    // wait a while before next loop
}

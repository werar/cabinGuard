/***
* TODO: temperature as float  maybe printf and linker options: -Wl,-u,vfprintf -lprintf_flt -lm   or: PRINTF_LIB_FLOAT = -Wl,-u,vfprintf -lprintf_flt -lm
* http://winavr.scienceprog.com/avr-gcc-tutorial/using-sprintf-function-for-float-numbers-in-avr-gcc.html
* TODO: check who is calling if known number arm/disarm sending alerts
* At+clip=1 or at+clcc during ring
* "AT#CID=1" - to enable caller ID
*
* TODO: use power off to save energy AT+CPWROFF
* TODO: fix function names convention  based on that? http://www.ganssle.com/misc/fsm.doc
* TODO: maybe via GPRS to send reports alerts as well?
* TODO: move to u8g2 https://github.com/olikraus/u8g2/wiki
* https://shortn0tes.blogspot.com/2016/05/neoway-m590-gprs-tutorial-sending-and.html (look at comments)
*/

#include <Arduino.h>
#include <Wire.h>                                                       // required by BME280 library
#include <BME280_t.h>
#include <SPI.h>
#include <U8g2lib.h>


#include "main.h"
#include "gsm_modem.h"

#define MYALTITUDE  150.50

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


void draw(void) {
  // graphic commands to redraw the complete screen should be placed here
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString( 0, 0, "Telemetry:");
  char str[15];
  sprintf(str, "%.1fC  ", parameters.temperature);
  u8x8.drawString( 0, 1, str);
  sprintf(str, "%d%%  ", parameters.humidity);
  u8x8.drawString( 0, 2, str);
  sprintf(str, "%.1fhPa  ", parameters.pressure);
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
  timers.sent_message--;
  timers.sent_telemetry_report--;
  timers.reenable_alerts--;
}

void reset_timers()
{
  timers.secounds=0;
  timers.sent_telemetry_report=SECOUNDS_TO_SEND_TELEMETRY_REPORT;
  timers.sent_message=SECOUNDS_TO_WAIT_WITH_ALERT_MESSAGE;
  timers.is_alert_was_sent=false;
  timers.reenable_alerts=SECOUNDS_TO_WAIT_WITH_ALERT_MESSAGE;
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
  bool disable_alerts=is_calling(PHONE_TO_UNARM,u8x8);
  if(disable_alerts)
  {
    parameters.enable_alert=false;
  }
  if(timers.reenable_alerts<=0)
  {
    parameters.enable_alert=true;
  }
  if(parameters.pir_alert && !timers.is_alert_was_sent && parameters.enable_alert)
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
}

void messages_and_reports()
{
  draw();
  if(timers.sent_telemetry_report<=0)
  {
    char text_to_sent[100];
    strcpy(text_to_sent,"Telemetry report\n");
    char buf[20];
    sprintf(buf, "Temperature: %.2f\n", (double)parameters.temperature);
    //sprintf(buf, "Temperature: %d\n",parameters.temperature);
    strcat(text_to_sent,buf);
    sprintf(buf, "Humidity: %d%%\n", parameters.humidity);
    strcat(text_to_sent,buf);
    sprintf(buf, "Pressure: %.2fhPa\n", parameters.pressure);
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
  init_GSM();
  delay(5000);
  BMESensor.begin();                                                    // initalize bme280 sensor
  init_avr_timers();
  reset_timers();
  parameters.enable_alert=true;

}

void loop()
{
   get_BMEData();
   check_movement();
   messages_and_reports();
   alerts();


   delay(10);                                                    // wait a while before next loop
}

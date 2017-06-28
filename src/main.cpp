#include <Arduino.h>

#include <Wire.h>                                                       // required by BME280 library
#include <BME280_t.h>
#include <AltSoftSerial.h>


#include "main.h"

//decrease  default modem speed: AT+IPR=9600

#define ASCII_ESC 27

#define MYALTITUDE  150.50



AltSoftSerial M590;

char bufout[10];

BME280<> BMESensor;                                                     // instantiate sensor

void printBMEData()
{
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

void initM50()
{
M590.begin(4800);
delay(5000);
M590.print("AT+CMGF=1\r");  // set SMS mode to text
Serial.println("set SMS mode to txt");  // set SMS mode to text
delay(2500);

M590.print("AT+CSCS=\"GSM\"");
M590.print("\r");
delay(2500);

Serial.println("set SMS mode to txt");  // set SMS mode to text
M590.print("AT+CMGF=1\r");  // set SMS mode to text
delay(2500);

 // blurt out contents of new SMS upon receipt to the GSM shield's serial out
Serial.println("blurt out contents of new SMS upon receipt to the GSM shield's serial out");
M590.print("AT+CNMI=2,2,0,0,0\r");
delay(2500);

 Serial.println("Ready...");
M590.println("AT+CMGD=1,4"); // delete all SMS
Serial.println("delete all SMS"); // delete all SMS
delay(2500);
M590.print("AT+CCID\r");

}

char phone_no[]="";

void send_sms(String message)
{
  Serial.write("AT+CREG?\r");
   delay(300);
  Serial.write("AT+CSCS=\"GSM\"\r");
  delay(300);
  Serial.write("AT+CMGF=1\r");    //text mode
  delay(2000);
  Serial.write("AT+CMGS=\"");
  Serial.write(phone_no);
  Serial.write(0x22);
  Serial.write(0x0D);  // hex equivalent of Carraige return
  Serial.write(0x0A);  // hex equivalent of newline
  delay(2000);
  Serial.print(message);
  delay(500);
  //Serial.write(0x1A);
  Serial.print(char(26));//the ASCII code of the ctrl+z is 26
}



void setup()
{
  Serial.begin(4800);                                                 // initialize serial
  pinMode(PIR_PIN, INPUT);


  //initM50();
  delay(5000);
    send_sms("Test");
  BMESensor.begin();                                                    // initalize bme280 sensor
}

void loop() {

   //printBMEData();
char c;

 if (Serial.available()) {
   c = Serial.read();
   M590.print(c);
 }
 if (M590.available()) {
   c = M590.read();
   Serial.print(c);
 }
   //delay(1000);                                                          // wait a while before next loop
}


/***
* TODO: check if someone is calling
* TODO: make uniwersal way to catch serial
*/

#include <Arduino.h>
#include "gsm_modem.h"

/***
* M590 chip MUST to be supplied near 4.2V! 3.3 is not enough!
*/
void init_GSM()
{
  Serial.write("AT+CREG?\r");
  delay(300);
  Serial.write("AT+CSCS=\"GSM\"\r");
  delay(300);
  Serial.write("AT+CMGF=1\r");    //text mode
  delay(2000);
}
 // blurt out contents of new SMS upon receipt to the GSM shield's serial out
//Serial.println("blurt out contents of new SMS upon receipt to the GSM shield's serial out");
//M590.print("AT+CNMI=2,2,0,0,0\r");
//delay(2500);

// Serial.println("Ready...");
//M590.println("AT+CMGD=1,4"); // delete all SMS
//Serial.println("delete all SMS"); // delete all SMS
//delay(2500);
//M590.print("AT+CCID\r");


void send_sms(char* message, const char* phone_no)
{
  #ifdef DEBUG
  Serial.println(message);
  #else
  Serial.write("AT+CMGS=\"");
  Serial.write(phone_no);
  Serial.write(0x22);
  Serial.write(0x0D);  // hex equivalent of Carraige return
  Serial.write(0x0A);  // hex equivalent of newline
  delay(2000);
  Serial.print(message);
  delay(500);
  Serial.print(char(26));//the ASCII code of the ctrl+z is 26
  #endif
}

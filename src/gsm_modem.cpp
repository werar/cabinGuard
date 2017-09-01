
/***
* TODO: check if someone is
* TODO: make uniwersal way to catch serial

https://dweet.io/faq
https://www.freeboard.io/board/edit/rBbZQR

*/

#include <Arduino.h>
#include "gsm_modem.h"
#include "main.h"


TinyGsm modem(SerialAT);
TinyGsmClient client(modem);


//#define DEBUG //uncomment that if you want to see messages on Serial console
/***
* M590 chip MUST to be supplied near 4.2V! 3.3 is not enough!
*/
void init_GSM()
{
   modem.restart();
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
  if (Serial.available() > 0)
  {
    Serial.readBytesUntil('\r', buffer, 100);
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
    if (!modem.waitForNetwork()) {
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

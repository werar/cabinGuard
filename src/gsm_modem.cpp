
/***
* TODO: check if someone is
* TODO: make uniwersal way to catch serial

https://dweet.io/faq
https://www.freeboard.io/board/edit/rBbZQR

*/

#include <Arduino.h>
#include "gsm_modem.h"
#include "main.h"



//#define DEBUG //uncomment that if you want to see messages on Serial console
/***
* M590 chip MUST to be supplied near 4.2V! 3.3 is not enough!
*/
void init_GSM()
{
  delay(5000); //needed to establish connecton with GSM network
  Serial.write("AT+CREG?\r");
  delay(300);
  Serial.write("AT+CSCS=\"GSM\"\r");
  delay(300);
  Serial.write("AT+CMGF=1\r");    //text mode
  delay(300);
  Serial.write("AT+CLIP=1\r");
  delay(300);
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


void send_sms(const char* message, const char* phone_no)
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
bool is_calling(char* caller_number,U8X8 u8x8)
{
  char buffer[50];//="OK\n\RING\n+CLIP: \"517083663\",129,,,\"\",0\nRING\+CLIP: \"517083663\",129,,,\"\",0";
  if (Serial.available() > 0)
  {
  Serial.readBytesUntil('\n', buffer, 50);
  if(&u8x8!=NULL)
  {
    u8x8.setFont(u8x8_font_victoriamedium8_r);
    u8x8.setCursor(0, 6);
    u8x8.print("               ");
    u8x8.print(buffer);
  }
  char * pch;
  pch = strstr(buffer,"+CLIP");
  if(pch!=NULL)
  {
    pch = strstr(buffer,caller_number);
    if(pch!=NULL) //looks like I call to the system
    { //action to diable/enable alerting
      PORTB ^= (1 << PB5);
      return true;
    }else{
      return false;
    }
  }
  return false;
  }
  return false;
}


/***
*
TODO: https://shortn0tes.blogspot.com/2016/05/neoway-m590-gprs-tutorial-sending-and.html
TODO: enable TCPIP  via: http://docs.mirifica.eu/Neoway.com/M590/Neoway%20M590%20AT%20Command%20Sets_V3.0.pdf
*/
void init_GPRS()
{
  write_and_wait("AT+XISP=0\r",500);
  write_and_wait("AT+cgdcont=1,\"IP\",\"internet\"\r",500);  //http://www.orange.pl/kid,4004241603,id,4004242390,title,Jakie-sa-prawidlowe-parametry-do-konfiguracji,helparticle.html
  write_and_wait("AT+XGAUTH=1,1,\"internet\",\"internet\"\r",1000);
  write_and_wait("AT+xiic=1\r",1000);
}

/***
*
https://forum.arduino.cc/index.php?topic=486845.0
http://www.arturnet.pl/modem-m590-obsluga-arduino-wywolujemy-restapi/
V
*/
void send_telemetry_report(const char* report)
{
  write_and_wait("AT+TCPSETUP=0,34.203.32.119,80\r",1000);  //use dweet.io / freeboard //TODO: IP as #define
  char buf[20];
  uint16_t string_lenght = strlen(report);
  sprintf(buf, "AT+TCPSEND=0,%d\r",string_lenght);
  write_and_wait(buf,1000);
  //write_and_wait("POST /dweet/for/werar1234?test=1 HTTP/1.1\r\nHost: dweet.io\r\nConnection: close\r\nAccept: */*\r\n\r\n",500); //http://www.esp8266.com/viewtopic.php?f=19&t=1981
  write_and_wait(report,500);
  write_and_wait((char)0x0D,1000);
  write_and_wait("AT+TCPCLOSE=1\r",500);
}

void write_and_wait(const char* text, uint16_t delay_time)
{
  Serial.write(text);
  delay(delay_time);
}


/***
* TODO: check if someone is
* TODO: make uniwersal way to catch serial

https://dweet.io/faq
https://www.freeboard.io/board/edit/rBbZQR

*/

#include <Arduino.h>
#include "gsm_modem.h"
#include "main.h"

//#define DEBUG

/***
*/
void init_GPRS()
{
  uint8_t number_of_try=5;
  delay(6000);
  while(number_of_try>0)
  {
      send_commands_to_init_GPRS();
      if(is_ppp_link_established())
      {
        return;
      }else
      {
        number_of_try--;
        delay(1000);
      }
  }
}

void send_commands_to_init_GPRS()
{
  write_and_wait("AT+XISP=0\r",500);
  write_and_wait("AT+cgdcont=1,\"IP\",\"internet\"\r",500);  //http://www.orange.pl/kid,4004241603,id,4004242390,title,Jakie-sa-prawidlowe-parametry-do-konfiguracji,helparticle.html
  write_and_wait("AT+XGAUTH=1,1,\"internet\",\"internet\"\r",1000);
  write_and_wait("AT+xiic=1\r",5000);
}

void update_time_from_provider()
{
  write_and_wait("AT+COPS=2\r",1000);
  write_and_wait("AT+CTZU=1\r",1000);
  write_and_wait("AT+COPS=0\r",1000);
  write_and_wait("AT+CCLK?\r",1000);

}

//#define DEBUG //uncomment that if you want to see messages on Serial console
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
  delay(300);
  Serial.write("AT+CLIP=1\r");
  delay(300);
  Serial.write("AT+CMGD=0,4\r"); //delete all smses
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
  init_GSM(); //TODO: still hard to catch the error, workaroud: restart evry time the
  prepare_and_send_sms(message, phone_no);
  /***
  If error will recived reinit GSM connection
  OK
  AT+CMGS="0048517083663"
  > Intruder inside the house!! Call to Elvis!␚
  >
  ERROR <<<<< here is the problem
  */

  const uint8_t buffer_size=50;
  char buffer[buffer_size]={"xxxxxxxxxx"};
  if (Serial.available() > 0)
  {
    Serial.readBytesUntil('\n', buffer, buffer_size);//echo skip it
    Serial.readBytesUntil('\n', buffer, buffer_size);
    //Serial.readBytesUntil('\n', buffer, buffer_size);
    //Serial.readBytesUntil('\n', buffer, buffer_size);
    //Serial.readBytesUntil('\n', buffer, buffer_size);
    //Serial.print("Buffer: ");
    //Serial.print(buffer);
    //Serial.println(" END.\r");
    char * pch;
    pch = strstr(buffer,"ERROR");
    if(pch!=NULL)
    {
      Serial.println("Error during sms sending, gsm initialization is required");
      init_GSM();
      prepare_and_send_sms(message, phone_no);
    }
    pch = strstr(buffer,"OK");
    if(pch!=NULL)
    {
      //Serial.println("Status was OK");
    }
  }
  #endif
}


void prepare_and_send_sms(const char* message, const char* phone_no)
{
  const uint8_t buffer_size=50;//TODO: refactoring needed.
  char buffer[buffer_size]={"xxxxxxxxxx"};
  Serial.write("AT+CMGS=\""); //after that can be an error
  if (Serial.available() > 0)
  {
    Serial.readBytesUntil('\n', buffer, buffer_size);//echo skip it
    Serial.readBytesUntil('\n', buffer, buffer_size);
    char * pch;
    pch = strstr(buffer,"ERROR");
    if(pch!=NULL)
    {
      Serial.println("Error during sms sending, gsm initialization is required");
      init_GSM();
    }
  }

  Serial.write(phone_no);
  Serial.write(0x22);
  Serial.write(0x0D);  // hex equivalent of Carraige return
  Serial.write(0x0A);  // hex equivalent of newline
  delay(2000);
  Serial.print(message);
  delay(500);
  Serial.println(char(26));//the ASCII code of the ctrl+z is 26
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
void send_telemetry_report(const char* report)
{
  if(!is_ppp_link_established())
  {
    init_GPRS();
  }
  init_GPRS(); //as workaroud until full error processing will be covered
  write_and_wait("AT+TCPSETUP=0,34.203.32.119,80\r",1000);  //use dweet.io / freeboard //TODO: IP as #define
  //can be response: +TCPSETUP:Error n
  char buf[25];
  uint16_t string_lenght = strlen(report);
  sprintf(buf, "AT+TCPSEND=0,%d\r",string_lenght);
  write_and_wait(buf,1000);
  //can be +TCPSEND: Buffr not enough,0
  //write_and_wait("POST /dweet/for/werar1234?test=1 HTTP/1.1\r\nHost: dweet.io\r\nConnection: close\r\nAccept: */*\r\n\r\n",500); //http://www.esp8266.com/viewtopic.php?f=19&t=1981
  write_and_wait(report,500);
  const char c = (char)0x0D;
  write_and_wait(&c,1000);
  write_and_wait("AT+TCPCLOSE=0\r",500);
}

void write_and_wait(const char* text, uint16_t delay_time)
{
  Serial.write(text);
  delay(delay_time);
}

/***
After some time (hour?) estalished ppp link/connection is closed. Initializatoin is required in that case.
Below example of problem:

+TCPSETUP:0,FAIL
AT+TCPSETUP=0,34.203.32.119,80  <<setup is ok but ppp link is down use XIIC to check that
OK
AT+TCPSEND=0,129
+TCPSEND:Error1  <<<<<< Is error ppp link is not set:
POST /dweet/for/werar1234?temp=24.16&hum=60&press=979.84&pir_alert=1 HTTP/1.1
ERROR <<<<<<<< is ERROR

to test if ppp link is ok use:
AT+XIIC?
+XIIC:    0, 0.0.0.0   <<<<<it means that ppp link is not ok

at+xiic?
+XIIC:    1, 10.224.178.69 <<<<< means that the link is set
*/
bool is_ppp_link_established()
{
  //write_and_wait("AT+IPSTATUS=0\r",500); //
  write_and_wait("AT+XIIC?\r",1000);
  //if wrong will be: +XIIC:    0, 0.0.0.0    (first 4 spaces and 0)
  //if good: +XIIC:    1, 10.224.178.69 (fist 4 spaces and 1)
  const uint8_t buffer_size=35;
  char buffer[buffer_size]={"xxxxxxxxxx"};
  uint8_t chars_in_line=0;
  if (Serial.available() > 0)
  {
    chars_in_line=Serial.readBytesUntil('\n', buffer, buffer_size); //first line is echo (AT+XIIC?!)
    chars_in_line=Serial.readBytesUntil('\n', buffer, buffer_size);//
  }
  if(chars_in_line==0)
  {
    Serial.println("No any line recived on UART");
  }else
  {
    //Serial.print("Buffer: ");
    //Serial.print(buffer);
    //Serial.println(" END.");
  }

  char * pch;
  pch = strstr(buffer,"+XIIC:    0,");
  //pch = strstr(buffer,"0.0.0.0");
  if(pch!=NULL)
  {
      //ppp is not established reinit link
      //Serial.println("PPP is not established - gprs init is required");
    return false;
  }else
  {
      //Serial.println("PPP is established");
    return true;
  }
}

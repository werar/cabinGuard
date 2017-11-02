/***
*
*/

#ifndef GSM_MODEM_H_
#define GSM_MODEM_H_

//define type of the GSM modem
//#define TINY_GSM_MODEM_SIM800
//#define TINY_GSM_MODEM_SIM900
#define TINY_GSM_MODEM_A7
//#define TINY_GSM_MODEM_M590

#include <TinyGsmClient.h>
#include "main.h"  //TODO: make gsm_modem more uniwersal avoid using structures related to some particular cases (parameters type)

void init_GSM();
bool send_sms(const char* message, const char* phone_no);
bool send_telemetry_report(const char* report);
bool is_calling(char* caller_number);
void mqttCallback(char* topic, byte* payload, unsigned int len);
boolean mqttConnect();
bool send_telemetry_report_to_mqtt(parameters_type* parameters);
#endif

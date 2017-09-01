/***
*
*/

#ifndef GSM_MODEM_H_
#define GSM_MODEM_H_

#define TINY_GSM_MODEM_SIM800
//#define TINY_GSM_MODEM_SIM900
//#define TINY_GSM_MODEM_A6
//#define TINY_GSM_MODEM_M590

#include <TinyGsmClient.h>

#define SerialAT Serial

void init_GSM();
bool send_sms(const char* message, const char* phone_no);
bool send_telemetry_report(const char* report);
bool is_calling(char* caller_number);
#endif

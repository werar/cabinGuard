/***
*
*/

#ifndef GSM_MODEM_H_
#define GSM_MODEM_H_

#include <U8g2lib.h>

void init_GSM();
void init_GPRS();
void send_sms(const char* message, const char* phone_no);
void send_telemetry_report(const char* report);
bool is_calling(char* caller_number,U8X8 u8x8);
void write_and_wait(const char* text, uint16_t delay);

#endif

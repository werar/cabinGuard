/***
*
*/
#ifndef GSM_MODEM_H_
#define GSM_MODEM_H_

#include <U8g2lib.h>

void init_GSM();
void send_sms(const char* message, const char* phone_no);
bool is_calling(char* caller_number,U8X8 u8x8);

#endif

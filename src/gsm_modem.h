/***
*
*/
#ifndef GSM_MODEM_H_
#define GSM_MODEM_H_

void init_GSM();
void send_sms(char* message, const char* phone_no);
bool is_calling(char* caller_number);

#endif

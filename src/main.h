#ifndef MAIN_H_
#define MAIN_H_

#define PIR_PIN 3

#define SECOUNDS_TO_SEND_TELEMETRY_REPORT 172800; //172800 sms every two days
#define SECOUNDS_TO_WAIT_WITH_ALERT_MESSAGE 600;
#define PHONE_TO_CALL 004

typedef struct parameters_type
{
      uint8_t temperature;
      uint8_t humidity;
      bool pir_alert;
      int pressure;
};

typedef struct timers_type
{
  long secounds;
  long sent_message;
  long sent_telemetry_report;
  bool is_alert_was_sent;
};

#endif

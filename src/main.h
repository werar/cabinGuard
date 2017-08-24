#ifndef MAIN_H_
#define MAIN_H_

#define PIR_PIN 3
#define SECOUNDS_TO_SEND_TELEMETRY_REPORT 60; //172800 sms every two days
#define SECOUNDS_TO_WAIT_WITH_ALERT_MESSAGE 600;
#define SECOUNDS_TO_ARM_ALERTS 600;
#define PHONE_TO_CALL "0048517083663"
#define PHONE_TO_UNARM "517083663"
#define ASCII_ESC 27
#define ALERT_MESSAGE_TEXT "Intruder inside the house!! Call to Elvis!"

typedef struct parameters_type
{
      float temperature;
      uint8_t humidity;
      bool pir_alert;
      bool enable_alert;
      float pressure;
};

typedef struct timers_type
{
  uint32_t secounds;
  int32_t sent_telemetry_report;
  int32_t reenable_alerts;
  bool is_alert_was_sent;
};



#endif

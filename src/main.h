#ifndef MAIN_H_
#define MAIN_H_

#define PIR_PIN 3



typedef struct parameters_type
{
      uint8_t temperature;
      uint8_t humidity;
      bool pir_alert;
      int pressure;
};

#endif

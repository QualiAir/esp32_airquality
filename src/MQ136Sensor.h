#ifndef MQ136_SENSOR_H
#define MQ136_SENSOR_H
#include "esp_adc/adc_oneshot.h" 

void mq136_init(adc_oneshot_unit_handle_t adc_handle_in);

void mq136_read(void);

#endif
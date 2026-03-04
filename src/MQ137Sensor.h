#ifndef MQ137_SENSOR_H
#define MQ137_SENSOR_H
#include "esp_adc/adc_oneshot.h" 

void mq137_init(adc_oneshot_unit_handle_t adc_handle_in);

void mq137_read(void);

#endif
#ifndef MQ137_SENSOR_H
#define MQ137_SENSOR_H
#include "esp_adc/adc_oneshot.h" 

void mq137_init(adc_oneshot_unit_handle_t adc_handle_in);

void mq137_read(void);
float send_mq137_sensor__ppm(void);
float mq137_calibrate_r0(int samples, int delay_ms);
#endif
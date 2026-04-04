#ifndef MQ136_SENSOR_H
#define MQ136_SENSOR_H
#include "esp_adc/adc_oneshot.h" 

void mq136_init(adc_oneshot_unit_handle_t adc_handle_in);

void mq136_read(void);
float send_mq136_sensor__ppm(void);
float mq136_calibrate_r0(int samples, int delay_ms);
#endif
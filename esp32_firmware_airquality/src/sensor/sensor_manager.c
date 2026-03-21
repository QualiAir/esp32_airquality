#include "sensor_manager.h"
#include "esp_adc/adc_oneshot.h"
#include "sensor/mq136/MQ136Sensor.h"
#include "sensor/mq137/MQ137Sensor.h"
#include "sensor/gp2y1010/GP2Y1010_DustSensor.h"
#include "sensor/bme680/BME680Sensor.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "time.h"
#include "esp_log.h"

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static const char *TAG = "**** sensor_manager ****"; //tag for logging

void sensor_manager_init(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc1_handle));
    //initializing sensors
    mq136_init(adc1_handle);
    mq137_init(adc1_handle);
    dust_sensor_init(adc1_handle);
    bme680_sensor_init();
}

SensorData sensor_manager_read(void) {
    SensorData data = { 0 };

    // read all sensors
    //mq136_read();
    //mq137_read();
    DustReading dust = dust_sensor_read();
    BME680Reading bme = bme680_sensor_read();
    
    // pass in
    data.temperature  = send_bme680_sensor__temperature(bme);
    data.humidity     = send_bme680_sensor__humidity(bme);
    data.pressure     = send_bme680_sensor__pressure(bme);
    data.dust_density = send_dust_sensor__density(dust);
    data.h2s_ppm      = send_mq136_sensor__ppm();
    data.nh3_ppm      = send_mq137_sensor__ppm();

    //timestamp
    time_t now;
    time(&now);
    data.timestamp = (int)now;
    ESP_LOGI(TAG, "timestamp is: %d", (int)now);

    return data;
}
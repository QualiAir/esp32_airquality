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
#include "freertos/FreeRTOS.h"  // for vTaskDelay
#include "freertos/task.h"      // for pdMS_TO_TICKS
#include "MQ136Sensor.h"        // for mq136_calibrate_r0

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static const char *TAG = "**** sensor_manager ****"; //tag for logging

// static void calibration_task(void *arg) {
//     vTaskDelay(pdMS_TO_TICKS(180000)); // wait 3 min warmup first!

//     //float r0_mq136 = mq136_calibrate_r0(50, 500);
//     float r0_mq137 = mq137_calibrate_r0(50, 500);
//     //ESP_LOGI(TAG, "MQ136 Calibrated R0: %.2f ohms", r0_mq136);
//     ESP_LOGI(TAG, "MQ137 Calibrated R0: %.2f ohms", r0_mq137);

//     vTaskDelete(NULL); // delete this task after calibration
// }

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

    // Create a task to perform sensor calibration after warmup
    //xTaskCreate(calibration_task, "calib_task", 4096, NULL, 3, NULL);
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
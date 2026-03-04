#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "MQ136Sensor.h"
#include "MQ137Sensor.h"
#include "GP2Y1010_DustSensor.h"
#include "driver/gpio.h" 
#include "BME680Sensor.h"


void app_main(void) {
    ESP_LOGI("VERSION", "ESP-IDF version: %s", esp_get_idf_version());


    // create ONE shared ADC1 handle for all sensors
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc1_handle));

    // initialize each sensor & Pass shared handle to each sensor
    mq136_init(adc1_handle);
    // mq137_init(adc1_handle);
    dust_sensor_init(adc1_handle);
    bme680_sensor_init();

    //     //test:
    // int raw = 0;
    // adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &raw);
    // ESP_LOGI("TEST", "Direct ADC read: %d", raw);

    // // Test: hold LED on continuously
    // gpio_set_direction(32, GPIO_MODE_OUTPUT);
    // while (1) {
    //     gpio_set_level(32, 1);
    //     ESP_LOGI("TEST", "GPIO32 HIGH");
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    while (1) {
        mq136_read();
        // mq137_read();
        DustReading reading = dust_sensor_read();
        dust_sensor_print(reading);
        BME680Reading bme = bme680_sensor_read();
        bme680_sensor_print(bme);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
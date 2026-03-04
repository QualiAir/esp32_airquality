#include "MQ136Sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <math.h>

static const char *TAG = "MQ136";

static const float Vcc = 5.0f;
static const float R1  = 10000.0f;  // 10k
static const float R2  = 20000.0f;  // 20k
static const float R0  = 10000.0f;  // calibrate experimentally in clean air

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t         cali_handle = NULL;

void mq136_init(adc_oneshot_unit_handle_t adc_handle_in) {
    adc1_handle = adc_handle_in;  // use shared handle, don't create a new one

    // DELETE the adc_oneshot_new_unit() block entirely

    // Keep channel config as-is:
    adc_oneshot_chan_cfg_t config = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config);

    // Keep cali config as-is:
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
}

void mq136_read(void) {
    int adc_raw    = 0;
    int voltage_mv = 0;

    adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_raw);
    adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_mv);

    float voltage_v = voltage_mv / 1000.0f;
    ESP_LOGI(TAG, "Raw ADC: %d | Voltage: %.3f V", adc_raw, voltage_v);

    if (voltage_v < 0.01f) {
        ESP_LOGW(TAG, "Voltage too low, sensor likely disconnected");
        return;
    }

    float Rs = (R2 * Vcc / voltage_v) - R1 - R2;
    ESP_LOGI(TAG, "Sensor Resistance: %.2f ohms", Rs);

    if (Rs <= 0) {
        ESP_LOGW(TAG, "Invalid Rs, skipping ppm calculation");
        return;
    }

    float m     = -0.35f;
    float b     =  0.8f;
    float ratio = Rs / R0;
    float ppm   = powf(10.0f, (log10f(ratio) - b) / m);
    ESP_LOGI(TAG, "H2S concentration: %.2f ppm", ppm);
}
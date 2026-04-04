#include "MQ136Sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <math.h>

//Using GPIO 34 (ADC1_CHANNEL_6) for MQ136 sensor input
static const char *TAG = "++++ MQ136 ++++";

static const float Vcc = 5.00f;
static const float R1  = 10000.0f;  // 10k Ohm - TOP sensor
static const float R2  = 20000.0f;  // 20k Ohm - BOTTOM sensor
static const float RL = 19530.0f; // Load resistor on MQ136 board
static const float R0  = 1666.57f;  // calibrate experimentally in clean air R0 = Rs/3.6, where rs = 3034.19(in clean air)

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t cali_handle = NULL;

void mq136_init(adc_oneshot_unit_handle_t adc_handle_in) {
    adc1_handle = adc_handle_in;  // use shared handle, don't create a new one

    // Keep channel config as-is:
    adc_oneshot_chan_cfg_t config = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config);

    // Keep cali config as-is:
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
}

float send_mq136_sensor__ppm(void){//mq136_read(void) {
    int adc_raw    = 0;
    int voltage_mv = 0;

    adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw);
    adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_mv);

    float voltage_v = voltage_mv / 1000.0f;
    ESP_LOGI(TAG, "Raw ADC: %d | Voltage: %.3f V", adc_raw, voltage_v);

    if (voltage_v < 0.01f) {
        ESP_LOGW(TAG, "Voltage too low, sensor likely disconnected");
        return -1.0f;
    }

    //float Rs = (R1 * Vcc / voltage_v) - R1 - R2;
    float V_sensor = voltage_v * ((R1 + R2) / R2);
    float Rs = (Vcc / V_sensor - 1.0f) * RL;
    ESP_LOGI(TAG, "Sensor Resistance: %.2f ohms", Rs);

    if (Rs <= 0) {
        ESP_LOGW(TAG, "Invalid Rs, skipping ppm calculation");
        return -1.0f;
    }

    float m     = -0.35f;
    float b     =  0.8f;
    float ratio = Rs / R0;
    float ppm   = powf(10.0f, (log10f(ratio) - b) / m);
    ESP_LOGI(TAG, "H2S concentration: %.2f ppm", ppm);
    return ppm;
}

// float send_mq136_sensor__ppm(void){
//     int adc_raw    = 0;
//     int voltage_mv = 0;

//     adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw);
//     adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_mv);

//     float voltage_v = voltage_mv / 1000.0f;
//     if (voltage_v < 0.01f) {
//         return -1.0f; // error code for invalid reading
//     }

//     float Rs = (R2 * Vcc / voltage_v) - R1 - R2;
//     if (Rs <= 0) {
//         return -1.0f; // error code for invalid reading
//     }

//     float m     = -0.35f;
//     float b     =  0.8f;
//     float ratio = Rs / R0;
//     float ppm   = powf(10.0f, (log10f(ratio) - b) / m);
//     return ppm;
// }


float mq136_calibrate_r0(int samples, int delay_ms)
{
    float rs_sum = 0.0f;
    int valid_samples = 0;

    for (int i = 0; i < samples; i++)
    {
        int adc_raw = 0;
        int voltage_mv = 0;

        adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw);
        adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_mv);

        float voltage_v = voltage_mv / 1000.0f;
        ESP_LOGI(TAG, "Raw: %d | Voltage: %.3fV", adc_raw, voltage_v);

        if (voltage_v < 0.01f) {
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
            continue;
        }

        // Correct for voltage divider
        float V_sensor = voltage_v * ((R1 + R2) / R2);
        // Calculate Rs
        float Rs = (Vcc / V_sensor - 1.0f) * RL;

        if (Rs > 0) {
            rs_sum += Rs;
            valid_samples++;
            ESP_LOGI(TAG, "Rs resistance: %.2f Ohm", Rs);
            ESP_LOGI(TAG, "ADC voltage (after divider): %.4f V", voltage_v);
            ESP_LOGI(TAG, "V_sensor (reconstructed):    %.4f V", V_sensor);
        }

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    if (valid_samples == 0) {
        ESP_LOGE(TAG, "Calibration failed: no valid samples");
        return -1.0f;
    }

    float Rs_avg = rs_sum / valid_samples;

    // Clean air ratio from hanwei MQ136 datasheet
    float R0_calculated = Rs_avg / 3.6f;

    ESP_LOGI(TAG, "Calibration complete");
    ESP_LOGI(TAG, "Average Rs: %.2f ohms", Rs_avg);
    ESP_LOGI(TAG, "Calculated R0: %.2f ohms", R0_calculated);

    return R0_calculated;
}
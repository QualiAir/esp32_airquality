#include "GP2Y1010_DustSensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_timer.h"
//#include "rom/ets_sys.h"    // ets_delay_us()

static const char *TAG = "---- GP2Y10 ----";

// ---- Timing constants (microseconds) ----
#define PULSE_BEFORE_SAMPLE_US   280    // Wait after LED on before sampling
#define PULSE_AFTER_SAMPLE_US     40    // Wait after sample before LED off
#define PULSE_PERIOD_US         9680    // Remainder of 10ms cycle
#define GP2Y_VOC 0.46f  // update after measuring your actual clean air voltage

// ---- ADC handle (new driver) ----
static adc_oneshot_unit_handle_t adc_handle = NULL;

// ---- Init ----
void dust_sensor_init(adc_oneshot_unit_handle_t adc_handle_in) {
    adc_handle = adc_handle_in;  // use shared handle

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DUST_LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(DUST_LED_GPIO, 0);  // LED off initially

    // Keep channel config:
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, DUST_ADC_CHANNEL, &chan_cfg));
    ESP_LOGI(TAG, "Dust sensor initialised — LED GPIO%d, ADC channel %d", DUST_LED_GPIO, DUST_ADC_CHANNEL);
}

static void delay_us(uint32_t us) {
    uint64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < us);
}

// ---- Read ----
DustReading dust_sensor_read(void) {
    int32_t total = 0;
    int raw = 0;

    for (int i = 0; i < DUST_NUM_SAMPLES; i++) {
        // Turn LED ON via NPN transistor
        gpio_set_level(DUST_LED_GPIO, 1);
        delay_us(PULSE_BEFORE_SAMPLE_US);   // 280µs before sampling

        adc_oneshot_read(adc_handle, DUST_ADC_CHANNEL, &raw);
        total += raw;

        delay_us(PULSE_AFTER_SAMPLE_US);    // 40µs after sampling
        gpio_set_level(DUST_LED_GPIO, 0);       // Turn LED OFF
        delay_us(PULSE_PERIOD_US);          // Wait rest of 10ms cycle
    }

    int avg_raw = (int)(total / DUST_NUM_SAMPLES);

    // Convert to voltage
    // ESP32 ADC: 12-bit (0–4095), 3.3V reference
    // Multiply by voltage divider ratio to recover actual sensor voltage
    float voltage = ((float)avg_raw / 4095.0f) * 3.3f * DUST_VOLTAGE_DIV;

    // Convert to dust density mg/m³ (Sharp datasheet approximation)
    //float dust_density = (0.17f * voltage) - 0.1f;
    float dust_density = 0.17f * (voltage - GP2Y_VOC);
    if (dust_density < 0.0f) dust_density = 0.0f;

    DustReading result = {
        .voltage      = voltage,
        .dust_density = dust_density,
        .raw_adc      = avg_raw,
    };
    dust_sensor_print(result);
    return result;
}

// ---- Print ----
void dust_sensor_print(DustReading reading) {
    ESP_LOGI(TAG, "Raw ADC: %d | Voltage: %.2fV | Dust: %.4f mg/m³",
             reading.raw_adc, reading.voltage, reading.dust_density);
}

// ---- Deinit ----
void dust_sensor_deinit(void) {
    if (adc_handle) {
        //adc_oneshot_del_unit(adc_handle); - this will break the other sensors using the same ADC handle, so we won't delete it here
        adc_handle = NULL;
    }
}

float send_dust_sensor__density(DustReading reading){
    return reading.dust_density;
}
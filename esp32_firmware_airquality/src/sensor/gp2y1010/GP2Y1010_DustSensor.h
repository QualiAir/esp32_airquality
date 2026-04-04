#ifndef GP2Y1010_DUST_SENSOR_H
#define GP2Y1010_DUST_SENSOR_H

#include <stdint.h>
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"

// ---- Configuration ----
// Change these to match your wiring
#define DUST_LED_GPIO       32      // GPIO connected to NPN base use to control LED by applying 5v instead of 3.3v from ESP32
#define DUST_ADC_UNIT       ADC_UNIT_1
#define DUST_ADC_CHANNEL    ADC_CHANNEL_7  // from pinout, GPIO35
#define DUST_NUM_SAMPLES    10      // nb of samples to average
#define DUST_VOLTAGE_DIV    1.5f    // Voltage divider ratio (three 10kΩ resistors = 0.33, so multiply by 1.5)

// ---- Structs ----
typedef struct {
    float voltage;        // Voltage at sensor Vo pin (V)
    float dust_density;   // Dust density (mg/m³), 0 if negative
    int   raw_adc;        // Raw averaged ADC value
} DustReading;
/**
 Initialise the dust sensor GPIO and ADC.
Call once in app_main() before any reads.
 */
void dust_sensor_init(adc_oneshot_unit_handle_t adc_handle_in);

/**
Take a single averaged dust reading.
DustReading struct with voltage, dust_density, and raw_adc.
 */
DustReading dust_sensor_read(void);

float send_dust_sensor__density(DustReading reading);

void dust_sensor_print(DustReading reading);
void dust_sensor_deinit(void);

#endif // DUST_SENSOR_H
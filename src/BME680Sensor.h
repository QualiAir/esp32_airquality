#ifndef BME680_SENSOR_H
#define BME680_SENSOR_H

#include <stdbool.h>

// ---- I2C Configuration ----
#define BME680_SDA_PIN      21
#define BME680_SCL_PIN      22
#define BME680_I2C_ADDR     0x76        // SDO → GND = 0x76, SDO → 3.3V = 0x77
#define BME680_I2C_PORT     I2C_NUM_0
#define BME680_I2C_FREQ_HZ  100000

// ---- Result Struct ----
typedef struct {
    float temperature;      // °C
    float humidity;         // %
    float pressure;         // hPa
    float gas_resistance;   // Ohms
    bool  valid;
} BME680Reading;

// ---- API ----
bool         bme680_sensor_init(void);
BME680Reading bme680_sensor_read(void);
void         bme680_sensor_print(BME680Reading reading);

#endif // BME680_SENSOR_H
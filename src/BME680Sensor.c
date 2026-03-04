#include "BME680Sensor.h"
#include "bme68x.h"         // Bosch API — bme68x.c/h/defs.h must be in src/

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const char *TAG = "BME680";

// ---- Internal state ----
static struct bme68x_dev bme;
static struct bme68x_conf conf;
static struct bme68x_heatr_conf heatr_conf;

// ============================================================
// Bosch API I2C interface callbacks
// These are the glue between the Bosch API and ESP-IDF I2C
// ============================================================

static BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data,
                                              uint32_t len, void *intf_ptr) {
    uint8_t dev_addr = *(uint8_t *)intf_ptr;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) i2c_master_read(cmd, reg_data, len - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, reg_data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(BME680_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK) ? BME68X_OK : BME68X_E_COM_FAIL;
}

static BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data,
                                               uint32_t len, void *intf_ptr) {
    uint8_t dev_addr = *(uint8_t *)intf_ptr;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, reg_data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(BME680_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK) ? BME68X_OK : BME68X_E_COM_FAIL;
}

static void bme68x_delay_us(uint32_t period_us, void *intf_ptr) {
    (void)intf_ptr;
    vTaskDelay(pdMS_TO_TICKS((period_us + 999) / 1000));  // convert µs → ms, round up
}

// ============================================================
// Init
// ============================================================
bool bme680_sensor_init(void) {
    // Configure I2C bus
    i2c_config_t i2c_conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = BME680_SDA_PIN,
        .scl_io_num       = BME680_SCL_PIN,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BME680_I2C_FREQ_HZ,
    };
    i2c_param_config(BME680_I2C_PORT, &i2c_conf);
    i2c_driver_install(BME680_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);

    // Wire up Bosch API interface
    static uint8_t dev_addr = BME680_I2C_ADDR;
    bme.read     = bme68x_i2c_read;
    bme.write    = bme68x_i2c_write;
    bme.delay_us = bme68x_delay_us;
    bme.intf     = BME68X_I2C_INTF;
    bme.intf_ptr = &dev_addr;
    bme.amb_temp = 25;  // assumed ambient temperature for gas heater calc

    // Initialise sensor (reads chip ID, reads calibration, soft resets)
    int8_t rslt = bme68x_init(&bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "bme68x_init failed: %d", rslt);
        return false;
    }

    // Configure oversampling — match original Arduino settings
    conf.filter  = BME68X_FILTER_SIZE_3;
    conf.odr     = BME68X_ODR_NONE;
    conf.os_hum  = BME68X_OS_2X;
    conf.os_pres = BME68X_OS_4X;
    conf.os_temp = BME68X_OS_8X;
    bme68x_set_conf(&conf, &bme);

    // Configure gas heater: 320°C for 150ms
    heatr_conf.enable     = BME68X_ENABLE;
    heatr_conf.heatr_temp = 320;
    heatr_conf.heatr_dur  = 150;
    bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme);

    ESP_LOGI(TAG, "BME680 initialised on SDA=%d SCL=%d addr=0x%02X",
             BME680_SDA_PIN, BME680_SCL_PIN, BME680_I2C_ADDR);
    return true;
}

// ============================================================
// Read
// ============================================================
BME680Reading bme680_sensor_read(void) {
    BME680Reading result = { 0 };
    result.valid = false;

    // Trigger one forced measurement
    bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);

    // Wait for measurement to complete
    uint32_t del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme)
                          + (heatr_conf.heatr_dur * 1000);
    bme.delay_us(del_period, bme.intf_ptr);

    // Fetch data
    struct bme68x_data data;
    uint8_t n_fields = 0;
    int8_t rslt = bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme);

    if (rslt != BME68X_OK || n_fields == 0) {
        ESP_LOGW(TAG, "No data available (rslt=%d, n_fields=%d)", rslt, n_fields);
        return result;
    }

    result.temperature   = data.temperature;
    result.humidity      = data.humidity;
    result.pressure      = data.pressure / 100.0f;  // Pa → hPa
    result.gas_resistance = data.gas_resistance;
    result.valid         = (data.status & BME68X_GASM_VALID_MSK) ? true : false;

    if (!result.valid) {
        ESP_LOGW(TAG, "Gas measurement not valid yet — heater still stabilising");
        result.valid = true;  // still return temp/hum/pres as valid
    }

    return result;
}

// ============================================================
// Print
// ============================================================
void bme680_sensor_print(BME680Reading reading) {
    if (!reading.valid) {
        ESP_LOGW(TAG, "Invalid reading");
        return;
    }
    ESP_LOGI(TAG, "Temperature:  %.2f °C",   reading.temperature);
    ESP_LOGI(TAG, "Humidity:     %.2f %%",   reading.humidity);
    ESP_LOGI(TAG, "Pressure:     %.2f hPa",  reading.pressure);
    ESP_LOGI(TAG, "Gas:          %.0f Ohms", reading.gas_resistance);
}
#include "mqtt_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "sensor/MQ136Sensor.h"
#include "sensor/MQ137Sensor.h"
#include "sensor/GP2Y1010_DustSensor.h"
#include "driver/gpio.h" 
#include "sensor/BME680Sensor.h"
#include "esp_timer.h"
#include "time.h"
#include <inttypes.h>

static const char *TAG = "**** mqtt_manager ****"; //tag for logging
static esp_mqtt_client_handle_t client;

/****************THIS IS FOR TEST PURPOSES - THIS CAN BE LATER REMOVED *****************/
static void mqtt_publish_task(void *pvParameters) {

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc1_handle));
    //initializing sensors
    mq136_init(adc1_handle);
    //mq137_init(adc1_handle);
    dust_sensor_init(adc1_handle);
    bme680_sensor_init();

    time_t now=0;
    int retry = 0;
    while (now < 1700000000  && retry < 10) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        retry++;
        ESP_LOGI(TAG, "Waiting for time sync... %d", (int)now);
    }

    while(1) {
        if(client != NULL) {
            // read all sensors
        DustReading dust = dust_sensor_read();
        BME680Reading bme = bme680_sensor_read();
        float temperature = send_bme680_sensor__temperature(bme);    // pass in
        float humidity = send_bme680_sensor__humidity(bme);       // pass in
        float pressure = send_bme680_sensor__pressure(bme);

        float dust_density = send_dust_sensor__density(dust);
        float h2s_ppm = send_mq136_sensor__ppm();
        time_t now;
        time(&now);
        ESP_LOGI(TAG, "now is: %d", (int)now);

        float nh3_ppm = 19;//send_mq137_sensor__ppm();<- DUMMY 19ppm for testing
        


        // format into JSON string
        char OurData[256];
        snprintf(OurData, sizeof(OurData),
            "{\"device_id\":\"sensor1\","
            "\"ammonia\":%.4f,"
            "\"hydrogen_sulfide\":%.4f,"
            "\"humidity\":%.2f,"//in %
            "\"temperature\":%.2f,"//in °C
            "\"dust\":%.4f,"//in mg/m³
            "\"timestamp\":%d,"
            "\"pressure\":%.2f}",//in hPa
            nh3_ppm,
            h2s_ppm,
            humidity,
            temperature,
            dust_density,
            (int)now,
            pressure
        );
            esp_mqtt_client_publish(client, "qualiair/test", OurData, 0, 1, 0);
            ESP_LOGI(TAG, "Published to qualiair/test");
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // publish every 5 seconds
    }
}

static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "MQTT event received: %d", event_id);
    //handle MQTT events such as connection, disconnection, message received, etc...
    esp_mqtt_event_handle_t event = event_data;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected.....");    
            xTaskCreate(mqtt_publish_task, "mqtt_publish", 4096, NULL, 5, NULL);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected.....");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT message received: topic=%.*s, data=%.*s", event->topic_len, event->topic, event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error occurred.....");
            break;case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribed to topic");
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "Unsubscribed from topic");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Message published successfully");
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "Connecting to broker...");
            break;
        default:
            ESP_LOGW(TAG, "Unknown MQTT event: %d", event_id);
            break;
    }
}
void mqtt_manager_init(void){
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://test.mosquitto.org",
        .credentials.client_id = "esp32_airquality_client",
    };
    client = esp_mqtt_client_init(&mqtt_cfg); //initialize MQTT client with configuration
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

}

void mqtt_manager_connect(void){
    ESP_LOGI(TAG, "Connecting to HiveMQ...");
    esp_mqtt_client_start(client); //start MQTT client to connect to broker
}



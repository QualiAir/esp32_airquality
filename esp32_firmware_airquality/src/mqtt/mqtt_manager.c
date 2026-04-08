#include "mqtt_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor_manager.h"
#include "time.h"
#include "secrets.h"
#include "esp_crt_bundle.h"
#include "wifi_manager.h"
#include <inttypes.h>
#include "led/led_manager.h"

static const char *TAG = "**** mqtt_manager ****"; //tag for logging
static esp_mqtt_client_handle_t client;
static TaskHandle_t publish_task_handle = NULL; // Task handle for MQTT publish task


static void mqtt_publish_task(void *pvParameters) {
 
    time_t now=0;
    int retry = 0;
    while (now < 1700000000  && retry < 10) {
        vTaskDelay( pdMS_TO_TICKS(1000));
        time(&now);
        retry++;
        ESP_LOGI(TAG, "Waiting for time sync... %d", (int)now);
    }

    while(1) {
        if(client != NULL) {
            SensorData data = sensor_manager_read();
            // format into JSON string
            char sensor_data[256];
            snprintf(sensor_data, sizeof(sensor_data),
                "{\"device_id\":\"%s\","
                "\"ammonia\":%.4f,"
                "\"hydrogen_sulfide\":%.4f,"
                "\"humidity\":%.2f,"//in %
                "\"temperature\":%.2f,"//in °C
                "\"dust\":%.4f,"//in mg/m³
                "\"timestamp\":%d,"
                "\"pressure\":%.2f}",//in hPa
                wifi_manager_get_device_id(),
                data.nh3_ppm,
                data.h2s_ppm,
                data.humidity,
                data.temperature,
                data.dust_density,
                data.timestamp,
                data.pressure
            );
            esp_mqtt_client_publish(client, MQTT_TOPIC, sensor_data, 0, 1, 0);
            led_manager_set_state(LED_MODE_MQTT_SENDING);
            ESP_LOGI(TAG, "Published to %s", MQTT_TOPIC);
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
            //this will protect against multiple tasks being created if MQTT reconnects multiple times
            if(publish_task_handle == NULL) {
                xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 8192, NULL, 7, &publish_task_handle);
            }
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
        .broker.address.uri = MQTT_BROKER_URI,
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
        .credentials.client_id = MQTT_CLIENT_ID,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
    };
    client = esp_mqtt_client_init(&mqtt_cfg); //initialize MQTT client with configuration
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void mqtt_manager_connect(void){
    ESP_LOGI(TAG, "Connecting to HiveMQ...");
    esp_mqtt_client_start(client); //start MQTT client to connect to broker
}



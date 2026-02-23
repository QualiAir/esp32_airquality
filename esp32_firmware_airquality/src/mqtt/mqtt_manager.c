#include "mqtt_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "**** mqtt_manager ****"; //tag for logging
static esp_mqtt_client_handle_t client;

/****************THIS IS FOR TEST PURPOSES - THIS CAN BE LATER REMOVED *****************/
static void mqtt_publish_task(void *pvParameters) {
    while(1) {
        if(client != NULL) {
            esp_mqtt_client_publish(client, "qualiair/test", "hello from esp32", 0, 1, 0);
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


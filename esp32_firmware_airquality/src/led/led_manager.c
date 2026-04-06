#include "led_manager.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "**** led_manager ****";
static led_mode_t current_mode = LED_MODE_OFF;

static void led_task(void *arg) {
    while (1) {
        switch (current_mode) {
            case LED_MODE_OFF:
                gpio_set_level(LED_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LED_MODE_MQTT_SENDING:
                // Single slow blink
                gpio_set_level(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(200));
                gpio_set_level(LED_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(800));
                break;

            case LED_MODE_WIFI_CONNECTING:
                // Fast blink
                gpio_set_level(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_set_level(LED_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LED_MODE_RESET:
                // Fast blink during reset hold
                gpio_set_level(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(250));
                gpio_set_level(LED_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
                break;

            default:
                gpio_set_level(LED_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}

void led_manager_init(void) {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);
    xTaskCreate(led_task, "led_task", 1024, NULL, 3, NULL);
    ESP_LOGI(TAG, "LED manager initialized");
}

void led_manager_set_state(led_mode_t new_state) {
    current_mode = new_state;
}

led_mode_t led_manager_get_state(void) {
    return current_mode;
}

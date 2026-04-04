#include "freertos/FreeRTOS.h"  // for vTaskDelay
#include "freertos/task.h"      // for pdMS_TO_TICKS
#include "driver/gpio.h"  //for reset button

#define RESET_BUTTON_GPIO    GPIO_NUM_0
#define RESET_HOLD_MS        5000
#define LED_PIN              GPIO_NUM_2

static const char *TAG = "**** reset  ****"; //tag for logging

static void IRAM_ATTR reset_button_isr(void *arg) {
    // notify task on button press
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(*(TaskHandle_t *)arg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void reset_button_task(void *arg) {
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RESET_BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,  // trigger on press
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(RESET_BUTTON_GPIO, reset_button_isr, &task_handle);

    while (1) {
        // wait for interrupt notification — task sleeps here, no CPU waste
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // button pressed — count hold time
        int hold_time = 0;
        while (gpio_get_level(RESET_BUTTON_GPIO) == 0 && hold_time < RESET_HOLD_MS) {
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(250));
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(250));
            hold_time += 500;
        }

        if (hold_time >= RESET_HOLD_MS) {
            ESP_LOGW(TAG, "Resetting WiFi credentials...");
            nvs_flash_erase();
            esp_restart();
        } else {
            ESP_LOGI(TAG, "Button released too early, ignoring...");
        }
    }
}   
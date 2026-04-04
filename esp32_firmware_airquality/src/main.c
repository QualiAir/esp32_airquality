#include "state/state_machine.h"
#include "esp_log.h"
#include "nvs_flash.h" //include NVS flash for storing Wi-Fi credentials and other data
#include "driver/gpio.h"  //for reset button
#include "freertos/FreeRTOS.h"  // for vTaskDelay
#include "freertos/task.h"      // for pdMS_TO_TICKS
#include "wifi/wifi_manager.h" //include wifi manager for Wi-Fi provisioning and connectivity

static const char *TAG = "**** main ****"; //tag for logging
bool isProvisioned = false;

//reset button definitions
#define RESET_BUTTON_GPIO    GPIO_NUM_0  // BOOT button
#define RESET_HOLD_MS        5000        // 5 second hold
#define LED_PIN              GPIO_NUM_2

static void reset_button_task(void *arg) {
    gpio_set_direction(RESET_BUTTON_GPIO, GPIO_MODE_INPUT);
    
    while (1) {
        // Check if button is pressed
        if (gpio_get_level(RESET_BUTTON_GPIO) == 0) {
            ESP_LOGW(TAG, "Reset button pressed, hold for 5 seconds to reset...");
            
            int hold_time = 0;
            
            // Count how long button is held
            while (gpio_get_level(RESET_BUTTON_GPIO) == 0 && hold_time < RESET_HOLD_MS) {
                // Fast blink while holding
                gpio_set_level(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(250));
                gpio_set_level(LED_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
                hold_time += 500;
            }
            
            // Was it held long enough?
            if (hold_time >= RESET_HOLD_MS) {
                ESP_LOGW(TAG, "Resetting WiFi credentials...");
                nvs_flash_erase();
                esp_restart();
            } else {
                ESP_LOGI(TAG, "Button released too early, ignoring...");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // check every 100ms
    }
}

void app_main(){
    // Start reset button task
    xTaskCreate(reset_button_task, "reset_btn", 2048, NULL, 5, NULL);

    /*UNCOMMENT DURING DEVELOPMENT ONLY - THIS TAKES TOO MUCH MEMORY*/
    esp_log_level_set("*", ESP_LOG_VERBOSE); 
    //esp_log_level_set("*", ESP_LOG_WARN);    // only warnings/errors
    printf("*************** app_main started ***************\n");

    // NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash is full or new version found, erasing...");
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
   
    //init wifi
    wifi_manager_init();

    //init state machine
    sm_init();
    sm_start(); //start the state machine, this will kick off the state machine logic and handle everything from there (Wi-Fi provisioning, MQTT connection, sensor reading, etc...)

}
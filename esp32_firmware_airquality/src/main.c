#include "state/state_machine.h"
#include "esp_log.h"
#include "nvs_flash.h" //include NVS flash for storing Wi-Fi credentials and other data
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_netif.h" //include esp_netif for network interface management
#include "esp_wifi.h" //include esp_wifi for Wi-Fi connectivity
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h" //include BLE provisioning scheme for Wi-Fi provisioning
#include "esp_sntp.h"// for time synchronization
#include "driver/gpio.h"  //for reset button
#include "freertos/FreeRTOS.h"  // for vTaskDelay
#include "freertos/task.h"      // for pdMS_TO_TICKS

static const char *TAG = "**** main ****"; //tag for logging
bool isProvisioned = false;
#define RESET_BUTTON_GPIO    GPIO_NUM_0  // BOOT button
#define RESET_HOLD_MS        5000        // 5 second hold
#define LED_PIN              GPIO_NUM_2


// handle events such as Wi-Fi connection, MQTT messages, etc...
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGI(TAG, "Event received: %s, ID: %d", event_base, event_id);

    if(event_base == WIFI_PROV_EVENT) {
        switch(event_id) {
            case WIFI_PROV_INIT:
                ESP_LOGI(TAG, "Provisioning manager initialized.....");
                break;

            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started.....");
                sm_transition(STATE_PROVISIONING);
                break;

            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                sm_transition(STATE_CONNECTING);
                break;
            }

            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed! Reason: %s",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wrong password" : "Network not found");
                sm_transition(STATE_ERROR);
                break;
            }

            case WIFI_PROV_END:
                ESP_LOGI(TAG, "Provisioning ended.....");
                wifi_prov_mgr_deinit();
                break;

            default:
                ESP_LOGW(TAG, "Unknown provisioning event ID: %d", event_id);
                break;
        }

    } else if(event_base == WIFI_EVENT) {
        switch(event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started.....");
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WiFi station connected.....");
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                if(isProvisioned) {
                    ESP_LOGW(TAG, "Disconnected from AP, retrying.....");
                    esp_wifi_connect();
                }
                break;

            default:
                break;
        }

    } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        isProvisioned = true;
        sm_transition(STATE_ONLINE);
    }
}

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
                
                // Rapid blink = resetting
                for (int i = 0; i < 6; i++) {
                    gpio_set_level(LED_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    gpio_set_level(LED_PIN, 0);
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                
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

    esp_log_level_set("*", ESP_LOG_VERBOSE);
    printf("*************** app_main started ***************\n");

    // NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash is full or new version found, erasing...");
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    
    // NETIF AND EVENT LOOP
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif, "Qualiair Device");
    
    // WIFI INIT
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // REGISTER EVENT HANDLERS
    esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);

    sm_init();

    // Let state machine init the provisioning manager first via STATE_UNPROVISIONED
    sm_transition(STATE_UNPROVISIONED);

    // NOW it's safe to check — manager is initialized
    wifi_prov_mgr_is_provisioned(&isProvisioned);

    if(isProvisioned){
        ESP_LOGI(TAG, "Already provisioned, connecting to WiFi...");
        wifi_prov_mgr_deinit();
        sm_transition(STATE_CONNECTING);
    }
    // else: state machine already called wifi_prov_mgr_start_provisioning()->so BLE is already working, nothing else needed
}
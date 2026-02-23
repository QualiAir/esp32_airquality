#include "state/state_machine.h"
#include "esp_log.h"
#include "nvs_flash.h" //include NVS flash for storing Wi-Fi credentials and other data
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_netif.h" //include esp_netif for network interface management
#include "esp_wifi.h" //include esp_wifi for Wi-Fi connectivity
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h" //include BLE provisioning scheme for Wi-Fi provisioning


static const char *TAG = "**** main ****"; //tag for logging
bool isProvisioned = false;

// handle events such as Wi-Fi connection, MQTT messages, etc...
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGI(TAG, "Event received: %s, ID: %d", event_base, event_id);
    //handle events and transition states
    if(event_base == WIFI_PROV_EVENT) {
        switch(event_id) {
            case WIFI_PROV_INIT:
                ESP_LOGI(TAG, "Provisioning manager initialized.....");
                break;
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Wi-Fi started.....");
                sm_transition(STATE_PROVISIONING);
                break;
            case WIFI_PROV_CRED_RECV:
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                sm_transition(STATE_CONNECTING);
                 break;
                break;
            case WIFI_PROV_CRED_FAIL: 
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                break;
            case WIFI_PROV_END:
                ESP_LOGI(TAG, "Provisioning ended.....");
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started.....");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "Disconnected from AP, retrying.....");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_BSS_RSSI_LOW:
                ESP_LOGW(TAG, "WiFi signal is weak.....");
                break;
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "Access point started.....");
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "Access point stopped.....");
                break;
            default:
                ESP_LOGW(TAG, "Unknown provisioning event.....");
                break;
        }
    } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        sm_transition(STATE_ONLINE);
    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected.....");
        sm_transition(STATE_ERROR);
    }
}

void app_main(){
    
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    printf("*************** app_main started ***************\n");

    /*
    * Initialize NVS flash, network interface, and event loop
    * These are necessary for Wi-Fi provisioning and connectivity
    */
    esp_err_t ret = nvs_flash_init(); //initialize NVS flash

    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash is full or new version found, erasing and reinitializing....");
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    
    // NETIF AND EVENT LOOP
    esp_netif_init(); //initialize network interface
    esp_event_loop_create_default(); //create default event loop
    esp_netif_t *netif = esp_netif_create_default_wifi_sta(); //create default Wi-Fi station interface
    esp_netif_set_hostname(netif, "Qualiair Device"); //set hostname before wi-fi starts
    
    // WIFI INIT AFTER NVS AND NETIF
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    /*
    * register event_handler for Wi-Fi provisioning and connectivity events
    * when a wi-fi event happens, it will call the event_handler function
    * and decide which state needs to be transitioned on the event received
    */
    esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);

    sm_init(); //initialize state machine
    
    //check if device is already provisioned - meaning has wi-fi credentials stored in NVS flash
    wifi_prov_mgr_is_provisioned(&isProvisioned);
    if(isProvisioned){
        sm_transition(STATE_CONNECTING); //if already provisioned, try connecting
    } else {
        sm_transition(STATE_UNPROVISIONED); //set initial state to unprovisioned
    }

}
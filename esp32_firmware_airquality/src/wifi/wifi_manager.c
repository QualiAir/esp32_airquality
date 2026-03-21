#include "state/state_machine.h"
#include "esp_log.h"
#include "nvs_flash.h" //include NVS flash for storing Wi-Fi credentials and other data
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_netif.h" //include esp_netif for network interface management
#include "esp_wifi.h" //include esp_wifi for Wi-Fi connectivity
#include "esp_bt.h" //include esp_bt for Bluetooth functionality (BLE provisioning)
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h" //include BLE provisioning scheme for Wi-Fi provisioning
#include "esp_sntp.h"// for time synchronization
#include "driver/gpio.h"  //for reset button
#include "freertos/FreeRTOS.h"  // for vTaskDelay
#include "freertos/task.h"      // for pdMS_TO_TICKS
#include "mdns.h" //for mDNS hostname

static const char *TAG = "**** wifi_manager ****";
static bool isProvisioned = false;
static char device_ip[32] = {0};
static char device_id[32] = {0};

static esp_err_t device_info_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data) {
    
    if(inbuf != NULL && inlen > 0) {
        char incoming[32] = {0};
        size_t copy_len = inlen < sizeof(incoming)-1 ? inlen : sizeof(incoming)-1;
        memcpy(incoming, inbuf, copy_len);
        if(strcmp(incoming, "OK") == 0){
            ESP_LOGI(TAG, "Received OK from android app, device is provisioned with the info");
            isProvisioned = true;

            wifi_prov_mgr_stop_provisioning();
            
            // Send confirmation back
            const char *ack = "{\"status\":\"acknowledged\"}";
            *outlen = strlen(ack);
            *outbuf = (uint8_t *)strdup(ack);
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "Received unexpected data on device_info endpoint: %s", incoming);
        }
    }
    
    // Respond with device info - device ID and IP address
    char response[128];
    snprintf(response, sizeof(response), "{\"device_id\":\"%s\",\"ip\":\"%s\"}", device_id, device_ip);
    *outlen = strlen(response);
    *outbuf = (uint8_t *)strdup(response); // Caller will free this buffer
    ESP_LOGI(TAG, "Responding with device info: %s", response);
    return ESP_OK;
}

void wifi_manager_register_endpoint(){
    wifi_prov_mgr_endpoint_create("device_info");
    wifi_prov_mgr_endpoint_register("device_info", device_info_handler, NULL);
    ESP_LOGI(TAG, "Device info endpoint registered");
}

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
                // Free BLE memory — no longer needed after provisioning
                esp_bt_controller_disable();
                esp_bt_controller_deinit();
                esp_bt_mem_release(ESP_BT_MODE_BLE);
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
                    ESP_LOGW(TAG, "Wifi station disconnected, retrying to connect.....");
                    esp_wifi_connect();
                }
                break;

            default:
                break;
        }

    } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { //THE ESP32 GOT AN IP
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        //get IP
        snprintf(device_ip, sizeof(device_ip), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", device_ip);
        //setup mdns
        static bool mdns_initialized = false;
        if (!mdns_initialized) {
            mdns_init();
            mdns_hostname_set(device_id);
            mdns_instance_name_set("QualiAir Device");
            mdns_initialized = true;
            ESP_LOGI(TAG, "mDNS initialized with hostname: %s.local", device_id);
        }
        isProvisioned = true;
        sm_transition(STATE_ONLINE);
    }
}

void wifi_manager_init(void) {

    // NETIF AND EVENT LOOP
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    
    // WIFI INIT
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    //generate device ID from MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    snprintf(device_id, sizeof(device_id), "qualiair_%02X:%02X:%02X", mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Device ID: %s", device_id);
    //set hostname
    esp_netif_set_hostname(netif, device_id);

    esp_wifi_start();

    // REGISTER EVENT HANDLERS
    esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
}

bool wifi_manager_is_provisioned(void) {
    return isProvisioned;
}

const char* wifi_manager_get_device_id(void) {
    return device_id;
}
#include "state_machine.h"
#include "esp_log.h"
#include "esp_netif.h" //include esp_netif for network interface management
#include "esp_wifi.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include "mqtt/mqtt_manager.h" //include MQTT manager for handling MQTT connections and events
#include "esp_sntp.h"

static device_state_t current_state = STATE_UNPROVISIONED; //default state machine
static const char *TAG = "**** state_machine ****"; //tag for logging
extern bool isProvisioned; //extern variable to check if device is already provisioned
const char *pop = "TEAM1abcd1234"; //proof of possession for secure provisioning, this is the PIN that the user will have to input on the provisioning app to allow provisioning with that specific phone

/*
 * this function automatically assigns the current state to UNPROVISIONED
 * since it is the default state when the device is powered on for the first time
 */
void sm_init() {
    ESP_LOGI(TAG, "Initializing state machine");
    current_state = STATE_UNPROVISIONED;
}

/*
 * this function is responsible for transitioning the device from one state to another
 * it takes in the new state as a parameter and updates the current state accordingly
 * it also logs the transition for debugging purposes
 */
void sm_transition(device_state_t new_state) {
    ESP_LOGI(TAG, "Transitioning from state %d to state %d.....", current_state, new_state);
    current_state = new_state;

    switch(current_state) {
        case STATE_UNPROVISIONED:
            ESP_LOGI(TAG, "Device is unprovisioned, starting BLE provisioning.....");
            //start BLE provisioning
            // config provisining manager with BLE scheme and event handler
            wifi_prov_mgr_config_t config = {
                .scheme = wifi_prov_scheme_ble,
                .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
            };
            //check for errors in initializing provisioning manager
            ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
            /*
            * Start provisioning service. 
            *   This will wait for a connection from a provisioning client until the provisioning is successful (or fails after retries).
            * The arguments are:
            * - security: the security level to use for provisioning. This can be one of the following:
            *     - WIFI_PROV_SECURITY_0: no security (plain-text communication)
            *     - WIFI_PROV_SECURITY_1: secure communication with X25519 key exchange, proof of possession (pop) based authentication, and AES-CTR encryption
            *     - WIFI_PROV_SECURITY_2: secure communication with SRP6a based authentication and key exchange, and AES-GCM encryption/decryption
            * - wifi_prov_sec_params: parameters required for the chosen security level. 
            *       This means that the client has to input a PIN (i.e, 2026) to allow authentication with that specific phone.
            *       For now we leave it at NULL
            * - service_name: the name that will appear in the list of bluetooth devices on the client's phone
            * - service_key: an optional key that can be used by the provisioning client to authenticate with the device during provisioning. 
            *       This can be NULL if not used.
            */

            
            wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, pop, "QualiAir Link", NULL);
            break;
        case STATE_PROVISIONING:
            ESP_LOGI(TAG, "Device is provisioning, waiting for Wi-Fi credentials.....");
            //waiting for the app to send Wi-Fi credentials
            break;
        case STATE_CONNECTING:
            ESP_LOGI(TAG, "Device is connecting to Wi-Fi.....");
            esp_wifi_connect(); // triggers connection using saved credentials
            break;
        case STATE_ONLINE:
            ESP_LOGI(TAG, "Device is online.....");
            if(!isProvisioned) {
                wifi_prov_mgr_stop_provisioning(); //stop BLE provisioning service since credentials have been received
            }

            //connected to Wi-Fi, start MQTT
            mqtt_manager_init(); //initialize MQTT manager
            mqtt_manager_connect(); //connect to MQTT broker
            break;
        case STATE_ERROR:
            ESP_LOGE(TAG, "Device is in error state.....");
            //log error, retry or restart device
            break;
        default:
            ESP_LOGW(TAG, "Unknown state.....");
            break;
    }
}

/*
 * this function is used to retrieve the current state of the device
 * it returns the current state as a device_state_t enum value
 */
device_state_t sm_get_state() {
    return current_state;
}
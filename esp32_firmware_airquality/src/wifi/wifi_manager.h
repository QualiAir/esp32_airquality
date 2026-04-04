#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

void wifi_manager_init(void);
bool wifi_manager_is_provisioned(void);
void wifi_manager_start_provisioning(void);
void wifi_manager_connect(void);
void wifi_manager_create_endpoint(void);
void wifi_manager_register_endpoint(void);
const char* wifi_manager_get_device_id(void);

#endif
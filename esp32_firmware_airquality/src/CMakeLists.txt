# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS 
        "main.c"
        "state/state_machine.c"
        "mqtt/mqtt_manager.c"
    INCLUDE_DIRS
        "."
        "state"
        "mqtt"
    REQUIRES 
        esp_event 
        esp_wifi 
        nvs_flash 
        esp_netif 
        wifi_provisioning 
        bt
        mqtt
)

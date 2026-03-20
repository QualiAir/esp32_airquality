# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS 
        "main.c"
        "state/state_machine.c"
        "mqtt/mqtt_manager.c"
        "sensor/bme680/bme68x.c" 
        "sensor/bme680/BME680Sensor.c"
        "sensor/mq136/MQ136Sensor.c"
        "sensor/mq137/MQ137Sensor.c"
        "sensor/gp2y1010/GP2Y1010_DustSensor.c"
        "sensor/sensor_manager.c"
    INCLUDE_DIRS
        "."
        "state"
        "mqtt"
        "sensor"
        "sensor/bme680"
        "sensor/mq136"
        "sensor/mq137"
        "sensor/gp2y1010"
    REQUIRES
        driver
        esp_event
        esp_wifi
        mbedtls
        nvs_flash 
        esp_netif 
        wifi_provisioning 
        bt
        mqtt
        esp_adc
        freertos
        esp_timer
        lwip
        newlib
        log 
)

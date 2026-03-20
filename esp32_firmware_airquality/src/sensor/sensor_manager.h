#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    float dust_density;
    float h2s_ppm;
    float nh3_ppm;
    int   timestamp;
} SensorData;

void sensor_manager_init();
SensorData sensor_manager_read();

#endif
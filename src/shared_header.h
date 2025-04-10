// shared_data.h
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdbool.h>

// Define a structure to hold sensor data
typedef struct struct_sensor_data {
    int dht_status;         // DHT11_OK, DHT11_CRC_ERROR, DHT11_TIMEOUT_ERROR
    int temperature;        // degrees Celsius
    int humidity;           // percentage
    float mq2_lpg_ppm;      // ppm
    float mq2_co_ppm;       // ppm
    float mq2_smoke_ppm;    // ppm
    bool motion_detected;   // true if motion detected since last send
} sensor_data_t;

#endif // SHARED_DATA_H
#ifndef SHARED_HEADER_H
#define SHARED_HEADER_H

#include <stdbool.h>
#include <stdint.h>
#include <math.h> // Include for NAN

// Match DHT11 status codes if defined in DHT.h, otherwise define basic ones
#ifndef DHT11_OK
#define DHT11_OK        0
#define DHT11_CRC_ERROR -1
#define DHT11_TIMEOUT_ERROR -2
#endif

typedef struct {
    int temperature;        // From DHT11
    int humidity;           // From DHT11
    int dht_status;         // DHT11_OK, DHT11_CRC_ERROR, DHT11_TIMEOUT_ERROR
    float mq2_lpg_ppm;      // From MQ2 (use NAN for invalid/error)
    float mq2_co_ppm;       // From MQ2 (use NAN for invalid/error)
    float mq2_smoke_ppm;    // From MQ2 (use NAN for invalid/error)
    bool motion_detected;   // From PIR
    // Add any other relevant flags if needed
} sensor_data_t;

#endif // SHARED_HEADER_H
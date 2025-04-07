#include "MQ2.h"
#include "DHT.h"
#include "mjd_hcsr501.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_log.h"

static const char *TAG = "SENSOR_MAIN";
MQ2 mq2_sensor;

#define MQ2_PIN 5   // GPIO for MQ2 sensor
#define DHT_GPIO GPIO_NUM_4  // 🔧 Change to your actual GPIO pin
#define TAG "DHT_SENSOR"

// MQ2 Sensor Task
void mq2_task(void *pvParameters) {
    mq2_init(&mq2_sensor, MQ2_PIN);
    mq2_begin(&mq2_sensor);
    
    while (1) {
        float *values = mq2_read(&mq2_sensor, true);
        if (values) {
            ESP_LOGI(TAG, "MQ2 -> LPG: %.2f ppm, CO: %.2f ppm, Smoke: %.2f ppm", 
                     values[0], values[1], values[2]);
        }
        vTaskDelay(pdMS_TO_TICKS(READ_DELAY));
    }
}

// DHT11 Sensor Task
void dht11_task(void *pvParameters) {
    int16_t temperature = 0, humidity = 0;

    while (1) {
        esp_err_t result = dht_read_data(DHT_TYPE_DHT11, DHT_GPIO, &humidity, &temperature);

        if (result == ESP_OK) {
            // Convert values from x10 to readable format
            ESP_LOGI(TAG, "Temperature: %d°C, Humidity: %d%%", temperature / 10, humidity / 10);
        } else {
            ESP_LOGE(TAG, "Failed to read from DHT11 sensor");
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // Read every 2 seconds
    }
}


void app_main(void) {
    xTaskCreate(&mq2_task, "mq2_task", 4096, NULL, 5, NULL);
    xTaskCreate(&dht11_task, "dht11_task", 4096, NULL, 5, NULL);
}
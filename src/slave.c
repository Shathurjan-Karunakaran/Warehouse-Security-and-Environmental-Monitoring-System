// src/slave.c
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "esp_mac.h"
// #include "driver/adc.h" // No longer needed here if MQ2.h includes new ones

// Component Headers
#include "DHT.h"
#include "MQ2.h"         // Using updated MQ2 library
#include "mjd_hcsr501.h"

// Shared Data Structure
#include "shared_header.h"

static const char *TAG = "SLAVE";

// --- Configuration ---
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#define SEND_INTERVAL_MS 5000 // Send data every 5 seconds

// --- GPIO Pins & ADC Configuration (!!! REVIEW/CHANGE THESE !!!) ---
#define DHT11_GPIO_PIN  GPIO_NUM_4
#define PIR_GPIO_PIN    GPIO_NUM_5

// MQ2 Sensor ADC Configuration:
#define MQ2_ADC_UNIT    ADC_UNIT_1 // Must use ADC1 with WiFi
#define MQ2_ADC_CHANNEL ADC_CHANNEL_6 // e.g., GPIO36 is ADC1_CHANNEL_0
// *** Use the NEW Attenuation Definition ***
#define MQ2_ADC_ATTEN   ADC_ATTEN_DB_12 // Equivalent to old DB_11, use this now

// --- Master MAC Address (!!! REPLACE THIS !!!) ---
static uint8_t master_mac_addr[] = {0xD8, 0xBC, 0x38, 0xE4, 0x1E, 0x20}; // MUST REPLACE

// --- Sensor Component Instances ---
MQ2 mq2_sensor;
mjd_hcsr501_config_t pir_config = { .data_gpio_num = PIR_GPIO_PIN };

// --- Global Variables ---
volatile bool motion_flag = false;
bool is_mq2_calibrated = false; // Flag to track if MQ2 calibration was successful
bool is_pir_initialized = false;

// --- ESP-NOW Send Callback ---
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGD(TAG, "Data sent successfully to " MACSTR, MAC2STR(mac_addr));
    } else {
        ESP_LOGW(TAG, "Data send failed to " MACSTR, MAC2STR(mac_addr));
    }
}

// --- WiFi & ESP-NOW Initialization ---
static void wifi_espnow_init(void) {
    // (Initialization code remains the same)
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Network Stack and Event Loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start()); // Start WiFi in STA mode

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    // Add Master Peer
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, master_mac_addr, ESP_NOW_ETH_ALEN);
    peer_info.ifidx = ESPNOW_WIFI_IF;
    peer_info.encrypt = false; // Set to true if using encryption
    esp_err_t add_peer_result = esp_now_add_peer(&peer_info);
    if (add_peer_result != ESP_OK && add_peer_result != ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGE(TAG, "Failed to add master peer: %s", esp_err_to_name(add_peer_result));
        esp_now_deinit(); // Cleanup ESP-NOW if peer add fails
        // Handle failure appropriately - maybe halt or retry
        return;
    } else if (add_peer_result == ESP_ERR_ESPNOW_EXIST) {
         ESP_LOGW(TAG, "Master peer " MACSTR " already exists.", MAC2STR(master_mac_addr));
    } else {
         ESP_LOGI(TAG, "Master peer " MACSTR " added.", MAC2STR(master_mac_addr));
    }

     // Print own MAC for debugging
     uint8_t self_mac[6];
     ESP_ERROR_CHECK(esp_read_mac(self_mac, ESP_MAC_WIFI_STA));
     ESP_LOGI(TAG, "Slave MAC Address: " MACSTR, MAC2STR(self_mac));

    ESP_LOGI(TAG, "WiFi and ESP-NOW Initialized.");
}

// --- Sensor Initialization ---
void sensors_init() {
    ESP_LOGI(TAG, "Initializing Sensors...");

    // DHT11 Initialization
    DHT11_init(DHT11_GPIO_PIN);
    ESP_LOGI(TAG, "DHT11 Initialized on GPIO %d.", DHT11_GPIO_PIN);

    // MQ2 Initialization (Using new ADC Driver)
    ESP_LOGI(TAG, "Initializing MQ2 Sensor on ADC Unit %d, Channel %d, Attenuation %d...",
             MQ2_ADC_UNIT, MQ2_ADC_CHANNEL, MQ2_ADC_ATTEN);

    // *** Call updated mq2_init ***
    esp_err_t mq2_init_ret = mq2_init(&mq2_sensor, MQ2_ADC_UNIT, MQ2_ADC_CHANNEL, MQ2_ADC_ATTEN);

    if (mq2_init_ret == ESP_OK) {
         ESP_LOGI(TAG, "MQ2 ADC Initialized successfully.");
         // MQ2 Calibration (Requires sensor pre-heating!)
         ESP_LOGW(TAG, "MQ2 requires pre-heating before calibration for accuracy!");
         ESP_LOGI(TAG, "Starting MQ2 Calibration (blocking)... Ensure clean air environment.");
         // mq2_begin now returns bool
         is_mq2_calibrated = mq2_begin(&mq2_sensor);
         if (is_mq2_calibrated) {
             ESP_LOGI(TAG, "MQ2 Calibrated successfully. Ro = %.3f kOhm", mq2_sensor.Ro);
         } else {
             ESP_LOGE(TAG, "MQ2 Calibration FAILED! Readings will not be available.");
             // Optionally deinit ADC if calibration fails and sensor is unusable
             // mq2_deinit(&mq2_sensor);
         }
    } else {
         ESP_LOGE(TAG, "MQ2 ADC Initialization FAILED! Error: %s", esp_err_to_name(mq2_init_ret));
         is_mq2_calibrated = false; // Ensure flag is false if init fails
    }


    // PIR Initialization
    esp_err_t pir_init_result = mjd_hcsr501_init(&pir_config);
    is_pir_initialized = (pir_init_result == ESP_OK && pir_config.isr_semaphore != NULL);
    if (is_pir_initialized) {
        if(xSemaphoreTake(pir_config.isr_semaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
             ESP_LOGI(TAG, "PIR cleared initial trigger.");
        }
        motion_flag = false; // Start with no motion detected
        ESP_LOGI(TAG, "PIR Initialized on GPIO %d.", PIR_GPIO_PIN);
    } else {
        ESP_LOGE(TAG, "PIR Initialization Failed on GPIO %d: %s", PIR_GPIO_PIN, esp_err_to_name(pir_init_result));
    }

    ESP_LOGI(TAG, "Sensor Initialization Complete.");
}


// --- Sensor Reading Task ---
void sensor_task(void *pvParameter) {
    sensor_data_t data_to_send;

    while(1) {
        // --- Prepare data structure with default error/invalid values ---
        memset(&data_to_send, 0, sizeof(sensor_data_t));
        data_to_send.temperature = -99;
        data_to_send.humidity = -99;
        data_to_send.mq2_lpg_ppm = NAN; // Use NAN as default invalid value
        data_to_send.mq2_co_ppm = NAN;
        data_to_send.mq2_smoke_ppm = NAN;
        //data_to_send.dht_status = STATUS_ERR_OTHER; // Assuming this exists
        data_to_send.motion_detected = motion_flag;

        // --- Read DHT11 ---
        struct dht11_reading dht_data = DHT11_read();
        data_to_send.dht_status = dht_data.status;
        if (dht_data.status == DHT11_OK) {
            data_to_send.temperature = dht_data.temperature;
            data_to_send.humidity = dht_data.humidity;
            ESP_LOGD(TAG, "DHT Read OK: T=%d C, H=%d %%", data_to_send.temperature, data_to_send.humidity);
        } else {
            ESP_LOGW(TAG, "DHT Read Failed. Status: %d", dht_data.status);
        }

        // --- Read MQ2 ---
        if (is_mq2_calibrated) { // Only read if ADC init and calibration were successful
            float* mq2_values = mq2_read(&mq2_sensor, false);
            if (mq2_values != NULL) {
                // Assign values; negative values indicate calculation errors
                data_to_send.mq2_lpg_ppm = mq2_values[0];
                data_to_send.mq2_co_ppm = mq2_values[1];
                data_to_send.mq2_smoke_ppm = mq2_values[2];

                ESP_LOGD(TAG, "MQ2 Read: LPG=%.2f ppm, CO=%.2f ppm, Smoke=%.2f ppm",
                         data_to_send.mq2_lpg_ppm < 0 ? NAN : data_to_send.mq2_lpg_ppm, // Print NAN if error
                         data_to_send.mq2_co_ppm < 0 ? NAN : data_to_send.mq2_co_ppm,
                         data_to_send.mq2_smoke_ppm < 0 ? NAN : data_to_send.mq2_smoke_ppm);

                // Log specific warnings for calculation errors if needed
                if(mq2_values[0] < 0) ESP_LOGW(TAG, "MQ2 LPG calculation error (Code %.1f)", mq2_values[0]);
                if(mq2_values[1] < 0) ESP_LOGW(TAG, "MQ2 CO calculation error (Code %.1f)", mq2_values[1]);
                if(mq2_values[2] < 0) ESP_LOGW(TAG, "MQ2 Smoke calculation error (Code %.1f)", mq2_values[2]);

            } else {
                 ESP_LOGW(TAG, "MQ2 Read Failed (mq2_read returned NULL).");
                 // Keep NAN values set earlier
            }
        } else {
            ESP_LOGD(TAG, "MQ2 Skipping read (sensor not calibrated or ADC init failed)");
            // Keep NAN values set earlier
        }

        // --- Check PIR ---
        if (is_pir_initialized) {
            if (xSemaphoreTake(pir_config.isr_semaphore, 0) == pdTRUE) {
                motion_flag = true;
                data_to_send.motion_detected = true;
                ESP_LOGI(TAG, "PIR Motion Detected!");
            } else {
                 data_to_send.motion_detected = motion_flag;
            }
        } else {
             data_to_send.motion_detected = false;
        }


        // --- Send Data via ESP-NOW ---
        esp_err_t result = esp_now_send(master_mac_addr, (uint8_t *)&data_to_send, sizeof(sensor_data_t));
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Data packet queued for sending via ESP-NOW.");
             if (motion_flag) {
                motion_flag = false; // Reset internal flag for next detection cycle
                ESP_LOGD(TAG, "Motion flag reset after successful send.");
            }
        } else {
            ESP_LOGE(TAG, "ESP-NOW send error: %s. Data not sent.", esp_err_to_name(result));
        }

        // --- Task Delay ---
        vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MS));
    }
}

// --- Main Application Entry Point ---
void app_main(void) {
    ESP_LOGI(TAG, "Starting Slave Application...");

    wifi_espnow_init();  // Initialize WiFi stack and ESP-NOW communication
    sensors_init();      // Initialize all connected sensors

    // Create the main sensor reading and data sending task
    xTaskCreate(sensor_task, "sensor_task", 3584, NULL, 5, NULL);

    ESP_LOGI(TAG, "Initialization complete. Sensor task started.");
    // Example of how to deinit MQ2 if needed (e.g., on shutdown command)
    // vTaskDelay(pdMS_TO_TICKS(60000)); // Run for a minute
    // mq2_deinit(&mq2_sensor); // Clean up ADC resources
}
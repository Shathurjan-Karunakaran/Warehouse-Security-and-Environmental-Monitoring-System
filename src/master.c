#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h> // For isnan

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "esp_mac.h"

#include "driver/gpio.h"
#include "mqtt_client.h"
#include "lwip/sockets.h" // For MQTT
#include "lwip/dns.h"
#include "lwip/netdb.h"

// -- Include Provided Libraries --
// OLED
#include "ssd1306.h"
#include "font8x8_basic.h"
// RFID
#include "rc522.h"
#include "rc522_spi.h"      // Use SPI driver for RC522
//#include "rc522_types.h"    // General types
//#include "rc522_picc.h"     // For PICC details like UID

// -- Include Shared Data Structure --
#include "shared_header.h"

static const char *TAG = "MASTER";

// --- Configuration ---
// WiFi
#define WIFI_SSID "Galaxy M310BAA"
#define WIFI_PASSWORD "gyvp7699ui"

// MQTT
#define MQTT_BROKER_URL "mqtt://test.mosquitto.org" // Public Mosquitto Broker
#define MQTT_PORT 1883
// Define specific topics
#define MQTT_TOPIC_TEMP         "esp32/warehouse/temperature"
#define MQTT_TOPIC_HUMIDITY     "esp32/warehouse/humidity"
#define MQTT_TOPIC_LPG          "esp32/warehouse/gas/lpg"
#define MQTT_TOPIC_CO           "esp32/warehouse/gas/co"
#define MQTT_TOPIC_SMOKE        "esp32/warehouse/gas/smoke"
#define MQTT_TOPIC_MOTION       "esp32/warehouse/motion"
#define MQTT_TOPIC_INSIDE_COUNT "esp32/warehouse/inside_count"
#define MQTT_TOPIC_RFID         "esp32/warehouse/rfid"
#define MQTT_TOPIC_ALERT        "esp32/warehouse/alert"
#define MQTT_TOPIC_STATUS       "esp32/warehouse/status" // General status/messages

// ESP-NOW
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF ESP_IF_WIFI_STA

// Hardware Pins
#define OLED_SCL_PIN GPIO_NUM_22
#define OLED_SDA_PIN GPIO_NUM_21
#define OLED_RST_PIN -1 // Often not needed or tied to VCC

#define RFID_MISO_PIN GPIO_NUM_19
#define RFID_MOSI_PIN GPIO_NUM_23
#define RFID_SCK_PIN  GPIO_NUM_18
#define RFID_SDA_PIN  GPIO_NUM_5 // *** SDA here usually means CS/SS for SPI ***
#define RFID_RST_PIN  GPIO_NUM_4

#define BUZZER_PIN GPIO_NUM_26
#define LED_PIN    GPIO_NUM_27

// Logic Thresholds & Timings
#define TEMPERATURE_THRESHOLD 60 // degrees C
#define HUMIDITY_THRESHOLD    85 // percent
#define SMOKE_PPM_THRESHOLD   500 // Adjust based on calibration/testing
#define UNAUTHORIZED_RFID_LIMIT 3
#define ALERT_BUZZER_DURATION_MS 10000 // 10 seconds
#define ALERT_LED_BLINK_DURATION_MS 10000 // 10 seconds
#define LED_BLINK_INTERVAL_MS 250
#define DISPLAY_MESSAGE_TIMEOUT_MS 5000 // How long to show status messages

// --- Global Handles and Variables ---
static rc522_handle_t rfid_scanner = NULL;
static SSD1306_t oled_dev;
static esp_mqtt_client_handle_t mqtt_client = NULL;

// Shared data protected by mutex
static sensor_data_t latest_sensor_data;
static SemaphoreHandle_t data_mutex = NULL;

// RFID and Occupancy Tracking
#define MAX_AUTHORIZED_IDS 10
typedef struct {
    char uid[RC522_PICC_UID_STR_BUFFER_SIZE_MAX];
    bool is_inside;
} authorized_person_t;

// !!! IMPORTANT: Populate this with your actual authorized tag UIDs !!!
authorized_person_t authorized_people[MAX_AUTHORIZED_IDS] = {
    {"XX XX XX XX", false}, // Example: Replace XX with hex bytes
    {"YY YY YY YY YY YY YY", false},
    // Add more authorized IDs here
};
static int num_authorized_people = 0; // Calculated based on non-empty UIDs in the list
static int inside_person_count = 0;

static char last_unauthorized_rfid_attempt[RC522_PICC_UID_STR_BUFFER_SIZE_MAX] = {0};
static int unauthorized_attempts_count = 0;

// Display & Status
static char display_line1[17] = "Temp: --- C";
static char display_line2[17] = "Humi: --- %";
static char display_line3[17] = "Inside: 0";
static char display_line4[17] = "Status: Init..."; // General status line
static TickType_t display_message_clear_time = 0;

// Network Status
static bool wifi_connected = false;
static bool mqtt_connected = false;

// --- Forward Declarations ---
static void wifi_init(void);
static void mqtt_app_start(void);
static void espnow_init(void);
static void oled_init(void);
static void rfid_init(void);
static void gpio_init(void);
static void alert_task(void *pvParameter);
static void oled_update_task(void *pvParameter);
static void activate_buzzer(uint32_t duration_ms); // Simple blocking version
static void blink_led(uint32_t duration_ms); // Simple blocking version
static void update_display_message(const char* message, bool temporary);
static void publish_mqtt_json(const char *topic, const char *json_payload); // Helper

// --- Event Handlers ---

// WiFi Event Handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Connecting to WiFi...");
        update_display_message("WiFi Connecting", false);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        mqtt_connected = false; // MQTT depends on WiFi
        ESP_LOGI(TAG, "WiFi disconnected. Retrying...");
        update_display_message("WiFi Lost", false);
        esp_wifi_connect();
        // Stop MQTT client? It should try reconnecting automatically if configured
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        update_display_message("WiFi OK", true); // Temporary message
        // Start MQTT client now that we have an IP
        if (mqtt_client == NULL) {
             mqtt_app_start();
        } else {
            esp_mqtt_client_start(mqtt_client); // Try to reconnect if already initialized
        }
    }
}

// MQTT Event Handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    // esp_mqtt_client_handle_t client = event->client; // Already global
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            update_display_message("MQTT OK", true);
             // Publish initial status or retained messages if needed
             publish_mqtt_json(MQTT_TOPIC_STATUS, "{\"status\": \"online\"}");
             // Publish current inside count on connect
             char json_buf[50];
             snprintf(json_buf, sizeof(json_buf), "{\"value\": %d}", inside_person_count);
             publish_mqtt_json(MQTT_TOPIC_INSIDE_COUNT, json_buf);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_connected = false;
            update_display_message("MQTT Lost", false);
            // The client will often try to reconnect automatically.
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA: // We don't expect to receive data in this example
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
             if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Last error code Reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGE(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            mqtt_connected = false; // Assume disconnected on error
            update_display_message("MQTT Error", false);
            break;
        default:
            ESP_LOGD(TAG, "Other MQTT event id:%d", event->event_id);
            break;
    }
}

// RFID Event Handler (Called by the RC522 library task)
// RFID Event Handler (Called by the RC522 library task)
// --- MODIFIED Function Signature and Logic ---
static void rfid_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data) {

    // Check if the event is the PICC state changing
    if (base == RC522_EVENTS && event_id == RC522_EVENT_PICC_STATE_CHANGED) {
        rc522_picc_state_changed_event_t* state_event_data = (rc522_picc_state_changed_event_t*)event_data;
        rc522_picc_t* picc = state_event_data->picc; // Get pointer to the PICC data

        // --- Process only when a tag becomes ACTIVE ---
        if (picc->state == RC522_PICC_STATE_ACTIVE || picc->state == RC522_PICC_STATE_ACTIVE_H) {
            // Check if this is a *new* activation event (optional, depends on library behavior)
            // You might only want to process on the transition *to* active state.
            // if (state_event_data->old_state != RC522_PICC_STATE_ACTIVE && state_event_data->old_state != RC522_PICC_STATE_ACTIVE_H) {
                ESP_LOGI(TAG, "Tag detected and selected (State: %d)", picc->state);

                char uid_str[RC522_PICC_UID_STR_BUFFER_SIZE_MAX];

                // Use the PICC data from the event
                if (rc522_picc_uid_to_str(&picc->uid, uid_str, sizeof(uid_str)) == ESP_OK) {
                    ESP_LOGI(TAG, "Tag Scanned UID: %s", uid_str);

                    // --- Rest of the authorization logic ---
                    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        bool authorized = false;
                        int person_index = -1;

                        // Find UID in authorized list
                        for (int i = 0; i < num_authorized_people; i++) {
                            if (strcmp(uid_str, authorized_people[i].uid) == 0) {
                                authorized = true;
                                person_index = i;
                                break;
                            }
                        }

                        if (authorized) {
                            char json_payload[150]; // Buffer for MQTT JSON

                            if (!authorized_people[person_index].is_inside) {
                                // Entry
                                authorized_people[person_index].is_inside = true;
                                inside_person_count++;
                                ESP_LOGI(TAG, "Authorized Entry: %s. Count: %d", uid_str, inside_person_count);
                                update_display_message("Entry OK", true);
                                blink_led(500);

                                // Publish MQTT Entry
                                snprintf(json_payload, sizeof(json_payload), "{\"status\": \"authorized\", \"action\": \"entry\", \"uid\": \"%s\"}", uid_str);
                                publish_mqtt_json(MQTT_TOPIC_RFID, json_payload);

                                // Publish updated count
                                char count_payload[30];
                                snprintf(count_payload, sizeof(count_payload), "{\"value\": %d}", inside_person_count);
                                publish_mqtt_json(MQTT_TOPIC_INSIDE_COUNT, count_payload);

                            } else {
                                // Exit
                                authorized_people[person_index].is_inside = false;
                                if (inside_person_count > 0) inside_person_count--;
                                ESP_LOGI(TAG, "Authorized Exit: %s. Count: %d", uid_str, inside_person_count);
                                update_display_message("Exit OK", true);

                                // Publish MQTT Exit
                                snprintf(json_payload, sizeof(json_payload), "{\"status\": \"authorized\", \"action\": \"exit\", \"uid\": \"%s\"}", uid_str);
                                publish_mqtt_json(MQTT_TOPIC_RFID, json_payload);

                                // Publish updated count
                                char count_payload[30];
                                snprintf(count_payload, sizeof(count_payload), "{\"value\": %d}", inside_person_count);
                                publish_mqtt_json(MQTT_TOPIC_INSIDE_COUNT, count_payload);
                            }
                            unauthorized_attempts_count = 0;
                            memset(last_unauthorized_rfid_attempt, 0, sizeof(last_unauthorized_rfid_attempt));

                        } else {
                            // Unauthorized
                            ESP_LOGW(TAG, "Unauthorized Tag: %s", uid_str);
                            char json_payload[150];
                            snprintf(json_payload, sizeof(json_payload), "{\"status\": \"unauthorized\", \"uid\": \"%s\"}", uid_str);
                            publish_mqtt_json(MQTT_TOPIC_RFID, json_payload);


                            if (strcmp(uid_str, last_unauthorized_rfid_attempt) == 0) {
                                unauthorized_attempts_count++;
                            } else {
                                unauthorized_attempts_count = 1;
                                strncpy(last_unauthorized_rfid_attempt, uid_str, sizeof(last_unauthorized_rfid_attempt) - 1);
                            }

                            if (unauthorized_attempts_count >= UNAUTHORIZED_RFID_LIMIT) {
                                ESP_LOGE(TAG, "ALERT: Multiple unauthorized attempts for %s!", uid_str);
                                update_display_message("ALERT: RFID", false);
                                activate_buzzer(ALERT_BUZZER_DURATION_MS);

                                // Publish MQTT Alert
                                snprintf(json_payload, sizeof(json_payload), "{\"source\": \"rfid\", \"reason\": \"multiple_unauthorized\", \"uid\": \"%s\"}", uid_str);
                                publish_mqtt_json(MQTT_TOPIC_ALERT, json_payload);

                                unauthorized_attempts_count = 0;
                                memset(last_unauthorized_rfid_attempt, 0, sizeof(last_unauthorized_rfid_attempt));
                            } else {
                                update_display_message("Access Denied", true);
                            }
                        }
                         xSemaphoreGive(data_mutex);
                    } else {
                        ESP_LOGE(TAG, "Could not get mutex in RFID handler");
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to convert scanned UID to string.");
                }
            // } // Optional: End check for new activation
        } else {
            // Log other state changes if needed (e.g., tag lost -> IDLE/HALT)
             ESP_LOGD(TAG, "PICC State Changed (Not Active): Old=%d, New=%d", state_event_data->old_state, picc->state);
        }
    } else {
         ESP_LOGW(TAG, "Unhandled event in rfid_handler: Base=%s, ID=%ld", base, event_id);
    }
}


// --- Initialization Functions ---

static void gpio_init(void) {
    gpio_config_t io_conf;

    // Configure Buzzer Pin
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << BUZZER_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(BUZZER_PIN, 0); // Off initially

    // Configure LED Pin
    io_conf.pin_bit_mask = (1ULL << LED_PIN);
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 0); // Off initially

    ESP_LOGI(TAG, "GPIO Initialized (Buzzer: %d, LED: %d)", BUZZER_PIN, LED_PIN);
}

static void oled_init(void) {
    // Use the legacy I2C driver functions exposed in ssd1306.h
    // The library handles selection between legacy/new based on IDF version internally
    i2c_master_init(&oled_dev, OLED_SDA_PIN, OLED_SCL_PIN, OLED_RST_PIN);
    ssd1306_init(&oled_dev, 128, 64); // Standard 128x64 resolution

    ssd1306_clear_screen(&oled_dev, false);
    ssd1306_display_text(&oled_dev, 0, "Initializing...", 15, false);
    vTaskDelay(pdMS_TO_TICKS(1000)); // Show init message
    ssd1306_clear_screen(&oled_dev, false);
    ESP_LOGI(TAG, "OLED Initialized (SDA: %d, SCL: %d)", OLED_SDA_PIN, OLED_SCL_PIN);
}

static void wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    // Set WiFi mode and config
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Adjust if needed
             .sae_pwe_h2e = WPA3_SAE_PWE_BOTH, // Enable WPA3 support if needed
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished.");
}

static void mqtt_app_start(void) {
     esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL, // Use this for IDF >= 5.0
        // .broker.address.port = MQTT_PORT,      // Use this for IDF >= 5.0
         //.uri = MQTT_BROKER_URL, // Use this for IDF < 5.0
         // Add credentials if needed:
         // .credentials.username = "user",
         // .credentials.authentication.password = "pass",
    };


    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
         ESP_LOGE(TAG, "Failed to initialize MQTT client");
         return;
    }
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
    ESP_LOGI(TAG, "MQTT Client Started.");
}
// --- MODIFIED ESP-NOW Receive Callback Signature ---
static void espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int len) {
    if (esp_now_info == NULL || esp_now_info->src_addr == NULL || data == NULL || len != sizeof(sensor_data_t)) {
        ESP_LOGW(TAG, "Received invalid ESP-NOW data.");
        return;
    }
    const uint8_t *mac_addr = esp_now_info->src_addr; // Extract MAC address
// --- End Modification ---

    // TODO: Optionally check if mac_addr matches the expected slave MAC

    ESP_LOGD(TAG, "Received data from " MACSTR, MAC2STR(mac_addr));

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        memcpy(&latest_sensor_data, data, sizeof(sensor_data_t));
        xSemaphoreGive(data_mutex);
        ESP_LOGD(TAG, "Sensor data updated: T=%d, H=%d, Motion=%d",
                 latest_sensor_data.temperature, latest_sensor_data.humidity, latest_sensor_data.motion_detected);
    } else {
        ESP_LOGE(TAG, "Could not obtain data mutex in ESP-NOW callback!");
    }
     // Immediately publish received data via MQTT if connected
    if (mqtt_connected) {
         char json_buf[100]; // Adjust size as needed

         // Temperature (handle invalid reading)
        if (latest_sensor_data.dht_status == DHT11_OK) {
            snprintf(json_buf, sizeof(json_buf), "{\"value\": %d}", latest_sensor_data.temperature);
            publish_mqtt_json(MQTT_TOPIC_TEMP, json_buf);
        } else {
             publish_mqtt_json(MQTT_TOPIC_TEMP, "{\"value\": null}"); // Publish null for error
        }


        // Humidity (handle invalid reading)
         if (latest_sensor_data.dht_status == DHT11_OK) {
            snprintf(json_buf, sizeof(json_buf), "{\"value\": %d}", latest_sensor_data.humidity);
            publish_mqtt_json(MQTT_TOPIC_HUMIDITY, json_buf);
        } else {
             publish_mqtt_json(MQTT_TOPIC_HUMIDITY, "{\"value\": null}"); // Publish null for error
        }


        // Gas sensors (handle NAN/error values)
        // Use temporary buffer for float formatting to avoid issues with snprintf and "null"
        char float_str[20];

        if (!isnan(latest_sensor_data.mq2_lpg_ppm) && latest_sensor_data.mq2_lpg_ppm >= 0) {
            sprintf(float_str, "%.2f", latest_sensor_data.mq2_lpg_ppm);
        } else {
            strcpy(float_str, "null");
        }
        snprintf(json_buf, sizeof(json_buf), "{\"value\": %s}", float_str);
        publish_mqtt_json(MQTT_TOPIC_LPG, json_buf);


        if (!isnan(latest_sensor_data.mq2_co_ppm) && latest_sensor_data.mq2_co_ppm >= 0) {
             sprintf(float_str, "%.2f", latest_sensor_data.mq2_co_ppm);
         } else {
             strcpy(float_str, "null");
         }
        snprintf(json_buf, sizeof(json_buf), "{\"value\": %s}", float_str);
        publish_mqtt_json(MQTT_TOPIC_CO, json_buf);


        if (!isnan(latest_sensor_data.mq2_smoke_ppm) && latest_sensor_data.mq2_smoke_ppm >= 0) {
             sprintf(float_str, "%.2f", latest_sensor_data.mq2_smoke_ppm);
         } else {
             strcpy(float_str, "null");
         }
        snprintf(json_buf, sizeof(json_buf), "{\"value\": %s}", float_str);
        publish_mqtt_json(MQTT_TOPIC_SMOKE, json_buf);


        // Motion
        snprintf(json_buf, sizeof(json_buf), "{\"value\": %s}", latest_sensor_data.motion_detected ? "true" : "false");
        publish_mqtt_json(MQTT_TOPIC_MOTION, json_buf);
    }
}
static void espnow_init(void) {
    // ESP-NOW should be initialized after WiFi has started
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_LOGI(TAG, "ESP-NOW Initialized for receiving.");
     // Print own MAC for debugging
     uint8_t self_mac[6];
     ESP_ERROR_CHECK(esp_read_mac(self_mac, ESP_MAC_WIFI_STA));
     ESP_LOGI(TAG, "Master MAC Address: " MACSTR, MAC2STR(self_mac));
}

static void rfid_init(void) {
    spi_bus_config_t bus_config = {
        .mosi_io_num = RFID_MOSI_PIN,
        .miso_io_num = RFID_MISO_PIN,
        .sclk_io_num = RFID_SCK_PIN,
        .quadwp_io_num = -1, // Not used
        .quadhd_io_num = -1, // Not used
        // .max_transfer_sz = 0 // Default is okay for RC522 usually
    };

    // *** Declare spi_config HERE ***
    rc522_spi_config_t spi_config = {
        .host_id = VSPI_HOST, // Or HSPI_HOST depending on ESP32 variant connection
        .dma_chan = SPI_DMA_CH_AUTO,
        .bus_config = &bus_config, // Pass pointer to bus config
        .dev_config = {
            .spics_io_num = RFID_SDA_PIN, // Chip Select pin
            .clock_speed_hz = 5 * 1000 * 1000, // 5 MHz
            .queue_size = 7,
            .mode = 0, // CPOL=0, CPHA=0 for RC522
            // .command_bits, .address_bits, .dummy_bits set by driver? Check rc522_spi.c
            .flags = SPI_DEVICE_HALFDUPLEX, // Necessary flag from rc522_spi.c
        },
        .rst_io_num = RFID_RST_PIN,
    };
    // *** End of spi_config declaration ***

    rc522_driver_handle_t spi_driver; // Handle for the lower-level SPI driver

    // *** Now use spi_config (this should be line 534 or similar) ***
    esp_err_t ret_spi_drv = rc522_spi_create(&spi_config, &spi_driver);
    if (ret_spi_drv != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RC522 SPI driver: %s", esp_err_to_name(ret_spi_drv));
        return; // Cannot continue without the driver
    }

    // --- Create the main RC522 configuration ---
    rc522_config_t config = {
        .driver = spi_driver, // Use the created SPI driver instance
        .poll_interval_ms = 500, // How often the library internally checks
        .task_stack_size = 4096,
        .task_priority = 4,
        .task_mutex = data_mutex, // Pass the mutex
    };

    // --- Create the RC522 handle (starts the internal task) ---
    esp_err_t ret_rc522 = rc522_create(&config, &rfid_scanner);
    if (ret_rc522 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RC522 instance: %s", esp_err_to_name(ret_rc522));
        // Consider cleanup: rc522_driver_destroy(spi_driver); ? Check library.
        return;
    }

    // --- Register event handler ---
    // Use RC522_EVENT_PICC_STATE_CHANGED for tag detection
    rc522_register_events(rfid_scanner, RC522_EVENT_PICC_STATE_CHANGED, rfid_handler, NULL);

    // --- Start the RC522 scanning process ---
    esp_err_t ret_start = rc522_start(rfid_scanner);
    if (ret_start != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start RC522 scanner: %s", esp_err_to_name(ret_start));
        // Consider cleanup: rc522_destroy(rfid_scanner); ? Check library.
        return;
    }

    ESP_LOGI(TAG, "RFID RC522 Initialized and scanning started.");

    // --- Calculate num_authorized_people ---
    num_authorized_people = 0;
    for (int i = 0; i < MAX_AUTHORIZED_IDS; ++i) {
        if (strlen(authorized_people[i].uid) > 0 && strcmp(authorized_people[i].uid, "XX XX XX XX") != 0 && strcmp(authorized_people[i].uid, "YY YY YY YY YY YY YY") != 0) { // Check against actual UIDs
            num_authorized_people++;
        } else if (strlen(authorized_people[i].uid) == 0) { // Stop at first truly empty slot
             break;
        }
    }
    ESP_LOGI(TAG, "Loaded %d authorized RFID UIDs.", num_authorized_people);
} // --- End of rfid_init ---

// --- Helper Functions ---

static void publish_mqtt_json(const char *topic, const char *json_payload) {
    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG, "MQTT not connected, cannot publish to %s", topic);
        return;
    }
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, json_payload, 0, 1, 0); // QoS 1
    if (msg_id != -1) {
         ESP_LOGD(TAG, "MQTT Published to %s: %s (msg_id=%d)", topic, json_payload, msg_id);
    } else {
         ESP_LOGE(TAG, "MQTT Failed to publish to %s", topic);
    }
}

// Simple blocking buzzer activation
static void activate_buzzer(uint32_t duration_ms) {
    ESP_LOGW(TAG, "Activating buzzer for %ld ms", duration_ms);
    gpio_set_level(BUZZER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    gpio_set_level(BUZZER_PIN, 0);
}

// Simple blocking LED blink
static void blink_led(uint32_t duration_ms) {
     ESP_LOGI(TAG, "Blinking LED for %ld ms", duration_ms);
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t duration_ticks = pdMS_TO_TICKS(duration_ms);
     TickType_t blink_interval_ticks = pdMS_TO_TICKS(LED_BLINK_INTERVAL_MS);

    while ((xTaskGetTickCount() - start_tick) < duration_ticks) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(blink_interval_ticks / 2);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(blink_interval_ticks / 2);
    }
     gpio_set_level(LED_PIN, 0); // Ensure LED is off after blinking
}

// Update the status line (line 4) of the display
static void update_display_message(const char* message, bool temporary) {
     strncpy(display_line4, message, sizeof(display_line4) - 1);
     display_line4[sizeof(display_line4)-1] = '\0'; // Ensure null termination

     if (temporary) {
         display_message_clear_time = xTaskGetTickCount() + pdMS_TO_TICKS(DISPLAY_MESSAGE_TIMEOUT_MS);
     } else {
         display_message_clear_time = 0; // 0 means don't clear automatically
     }
     // The actual display update happens in oled_update_task
}

// --- Tasks ---

// Task to periodically update the OLED display
static void oled_update_task(void *pvParameter) {
    char smoke_msg[17] = ""; // Buffer for smoke message
    char motion_msg[17] = ""; // Buffer for motion message

    while (1) {
        if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Format sensor data lines
             if(latest_sensor_data.dht_status == DHT11_OK) {
                snprintf(display_line1, sizeof(display_line1), "Temp: %d C", latest_sensor_data.temperature);
                snprintf(display_line2, sizeof(display_line2), "Humi: %d %%", latest_sensor_data.humidity);
             } else {
                snprintf(display_line1, sizeof(display_line1), "Temp: ERR");
                snprintf(display_line2, sizeof(display_line2), "Humi: ERR");
             }

            // Format inside person count
            snprintf(display_line3, sizeof(display_line3), "Inside: %d", inside_person_count);

             // Check for temporary message timeout
            if (display_message_clear_time != 0 && xTaskGetTickCount() >= display_message_clear_time) {
                strcpy(display_line4, "Status: OK"); // Default status
                display_message_clear_time = 0;
            }

            // Check for persistent smoke/motion messages only if status line is default
            if (strcmp(display_line4, "Status: OK") == 0) {
                 // Check smoke status from sensor data
                bool smoke_detected = !isnan(latest_sensor_data.mq2_smoke_ppm) &&
                                  latest_sensor_data.mq2_smoke_ppm >= SMOKE_PPM_THRESHOLD;

                 if (smoke_detected) {
                    snprintf(smoke_msg, sizeof(smoke_msg), "SMOKE DETECTED!");
                 } else {
                    smoke_msg[0] = '\0'; // Clear message
                 }

                  // Check motion status
                  // Only show motion alert if warehouse is supposed to be empty
                 bool motion_alert = (inside_person_count == 0 && latest_sensor_data.motion_detected);
                  if (motion_alert) {
                    snprintf(motion_msg, sizeof(motion_msg), "MOTION ALERT!");
                  } else {
                    motion_msg[0] = '\0'; // Clear message
                  }

                 // Prioritize smoke message over motion on line 4 if both occur
                 if(strlen(smoke_msg) > 0) {
                     strncpy(display_line4, smoke_msg, sizeof(display_line4) -1);
                      display_line4[sizeof(display_line4)-1] = '\0';
                 } else if (strlen(motion_msg) > 0) {
                      strncpy(display_line4, motion_msg, sizeof(display_line4) -1);
                      display_line4[sizeof(display_line4)-1] = '\0';
                 }
                 // If neither, it remains "Status: OK"
            }


            xSemaphoreGive(data_mutex);

             // Update OLED
            ssd1306_clear_screen(&oled_dev, false);
            ssd1306_display_text(&oled_dev, 0, display_line1, strlen(display_line1), false);
            ssd1306_display_text(&oled_dev, 1, display_line2, strlen(display_line2), false);
            ssd1306_display_text(&oled_dev, 2, display_line3, strlen(display_line3), false);
            ssd1306_display_text(&oled_dev, 3, display_line4, strlen(display_line4), false); // Display status/alert


        } else {
            ESP_LOGE(TAG, "OLED Task: Failed to get mutex!");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Update display every second
    }
}

// Task to check sensor values for alerts
// Task to check sensor values for alerts
static void alert_task(void *pvParameter) {
    bool temp_alert_active = false;
    bool humi_alert_active = false;
    bool smoke_alert_active = false;
    bool pir_alert_active = false;

   while(1) {
        sensor_data_t current_data; // Declare here
        int current_inside_count = 0; // Initialize count too
        bool trigger_buzzer = false;
        bool trigger_led_blink = false;
        char alert_reason[50] = "";

        // ****** ADD INITIALIZATION HERE ******
        // Initialize current_data with safe defaults before trying to read
        //memset(¤t_data, 0, sizeof(sensor_data_t)); // Zero out the structure
        current_data.temperature = -99; // Indicate no valid reading yet this loop
        current_data.humidity = -99;
        current_data.mq2_lpg_ppm = NAN;
        current_data.mq2_co_ppm = NAN;
        current_data.mq2_smoke_ppm = NAN;
        current_data.dht_status = DHT11_TIMEOUT_ERROR; // Assume error initially this loop
        current_data.motion_detected = false;
        // ****** END INITIALIZATION ******


        if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Try to copy the latest data
            // memcpy(¤t_data, &latest_sensor_data, sizeof(sensor_data_t));
            current_inside_count = inside_person_count; // Read count while holding mutex
            xSemaphoreGive(data_mutex);

            // --- Now checks are safe, operating on either latest data or defaults ---

            // --- Check Temperature Alert ---
            if (current_data.dht_status == DHT11_OK && current_data.temperature >= TEMPERATURE_THRESHOLD) {
                if (!temp_alert_active) {
                     ESP_LOGE(TAG, "ALERT: High Temperature! %d C", current_data.temperature);
                     trigger_buzzer = true;
                     temp_alert_active = true;
                     snprintf(alert_reason, sizeof(alert_reason), "high_temperature");
                     update_display_message("ALERT: TEMP HIGH", false);
                }
            } else {
                temp_alert_active = false;
            }

            // --- Check Humidity Alert ---
             if (current_data.dht_status == DHT11_OK && current_data.humidity >= HUMIDITY_THRESHOLD) {
                if (!humi_alert_active) {
                    ESP_LOGE(TAG, "ALERT: High Humidity! %d %%", current_data.humidity);
                    trigger_buzzer = true; // Add to buzzer trigger
                    humi_alert_active = true;
                     snprintf(alert_reason, sizeof(alert_reason), "high_humidity"); // Overwrites previous if both occur
                    update_display_message("ALERT: HUMI HIGH", false);
                }
             } else {
                humi_alert_active = false;
             }

             // --- Check Smoke Alert ---
             bool smoke_detected_now = !isnan(current_data.mq2_smoke_ppm) && current_data.mq2_smoke_ppm >= SMOKE_PPM_THRESHOLD;
             if (smoke_detected_now) {
                  if (!smoke_alert_active) {
                    ESP_LOGE(TAG, "ALERT: Smoke Detected! %.2f PPM", current_data.mq2_smoke_ppm);
                    trigger_buzzer = true; // Add to buzzer trigger
                    smoke_alert_active = true;
                     snprintf(alert_reason, sizeof(alert_reason), "smoke_detected"); // Overwrites previous
                    // OLED message handled by oled_update_task based on data
                }
             } else {
                smoke_alert_active = false;
             }

             // --- Check PIR Alert (Empty Warehouse) ---
             // Use the inside count read while mutex was held
             if (current_inside_count == 0 && current_data.motion_detected) {
                  if (!pir_alert_active) {
                     ESP_LOGE(TAG, "ALERT: Motion detected while warehouse empty!");
                     trigger_buzzer = true; // Will use timed buzzer later
                     trigger_led_blink = true;
                     pir_alert_active = true;
                     snprintf(alert_reason, sizeof(alert_reason), "motion_when_empty");
                     // OLED message handled by oled_update_task based on data
                 }
             } else {
                  // Reset PIR alert flag only when condition is no longer met
                  pir_alert_active = false;
             }

             // --- Trigger Alerts ---
             if (trigger_buzzer) {
                 // Special handling for PIR alert buzzer duration
                 if (pir_alert_active) {
                     activate_buzzer(ALERT_BUZZER_DURATION_MS);
                 } else {
                     activate_buzzer(1000); // Shorter buzz for temp/humi/smoke
                 }
             }

             if (trigger_led_blink) {
                 blink_led(ALERT_LED_BLINK_DURATION_MS);
                 // After blinking, reset the motion detected flag in shared data
                 // Need to take mutex again for this write operation
                  if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                      latest_sensor_data.motion_detected = false; // Reset the *shared* flag
                      xSemaphoreGive(data_mutex);
                      ESP_LOGI(TAG, "Reset motion flag after PIR alert handled.");
                  } else {
                       ESP_LOGE(TAG, "Failed to get mutex to reset motion flag");
                  }
             }

            // --- Publish Alert to MQTT (only if reason is set) ---
            if (strlen(alert_reason) > 0 && mqtt_connected) {
                 char alert_payload[100];
                 snprintf(alert_payload, sizeof(alert_payload), "{\"source\": \"sensor\", \"reason\": \"%s\"}", alert_reason);
                 publish_mqtt_json(MQTT_TOPIC_ALERT, alert_payload);
            }

        } else {
             ESP_LOGE(TAG, "Alert Task: Failed to get mutex! Checks skipped this cycle.");
             // When mutex fails, the checks will use the default initialized values,
             // which should prevent alerts based on uninitialized data.
        }

       vTaskDelay(pdMS_TO_TICKS(500)); // Check alerts every 500ms
   }
}

// --- Main ---
void app_main(void) {
    ESP_LOGI(TAG, "Starting Master Application...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create Mutex
    data_mutex = xSemaphoreCreateMutex();
    if (data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create data mutex!");
        return; // Cannot proceed
    }

    // Initialize hardware and communication
    gpio_init();
    oled_init();
    wifi_init();     // Initializes WiFi and starts connection attempt
    espnow_init();   // Initialize ESP-NOW receiver (after WiFi stack is up)
    // MQTT init is called by the WiFi event handler upon getting IP
    rfid_init();     // Initialize RFID (depends on SPI, GPIO)


    // Initialize sensor data structure
    memset(&latest_sensor_data, 0, sizeof(sensor_data_t));
    latest_sensor_data.temperature = -99; // Indicate no data yet
    latest_sensor_data.humidity = -99;
    latest_sensor_data.mq2_lpg_ppm = NAN;
    latest_sensor_data.mq2_co_ppm = NAN;
    latest_sensor_data.mq2_smoke_ppm = NAN;
    latest_sensor_data.dht_status = DHT11_TIMEOUT_ERROR; // Assume error initially
    latest_sensor_data.motion_detected = false;


    // Create Tasks
    xTaskCreate(oled_update_task, "oled_task", 3072, NULL, 5, NULL);
    xTaskCreate(alert_task, "alert_task", 3072, NULL, 4, NULL);


    ESP_LOGI(TAG, "Initialization Complete. System Running.");

     // The RFID scanning is handled by the rc522 library's internal task.
     // We just need to handle the events it generates via rfid_handler.
}
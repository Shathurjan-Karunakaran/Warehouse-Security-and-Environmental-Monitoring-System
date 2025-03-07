#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_AP_SSID "Master_AP"
#define WIFI_AP_PASS "password123"
#define WIFI_STA_SSID "Galaxy M310BAA"
#define WIFI_STA_PASS "gyvp7699ui"
#define MQTT_BROKER    "mqtts://b59f9b2fc3a74f45a790d2a27895c29e.s1.eu.hivemq.cloud:8883" // HiveMQ Cloud

// Log Tags
static const char *TAG = "ESP_MAIN";

esp_mqtt_client_handle_t mqtt_client;

// Wi-Fi Event Group
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

// Wi-Fi Event Handler
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_AP_STACONNECTED) {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG, "Station " MACSTR " joined, AID=%d", MAC2STR(event->mac), event->aid);
        } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG, "Station " MACSTR " left, AID=%d", MAC2STR(event->mac), event->aid);
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGI(TAG, "Disconnected, retrying...");
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Connected to Internet");
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_ap_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Register Wi-Fi event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .password = WIFI_AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = WIFI_STA_SSID,
            .password = WIFI_STA_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta);
    esp_wifi_start();
    esp_wifi_connect();

    ESP_LOGI(TAG, "Wi-Fi AP+STA Mode Initialized");
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            esp_mqtt_client_subscribe(mqtt_client, "master/control", 0);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Received message on topic: %.*s", event->topic_len, event->topic);
            break;
        default:
            break;
    }
}
// MQTT Initialization
void mqtt_app_start() {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER,
        .credentials.username = "master",   // Change to your HiveMQ username
        .credentials.authentication.password = "master@2001",   // Change to your HiveMQ password
        .broker.verification.skip_cert_common_name_check = true, // Use NULL if HiveMQ Cloud uses a globally trusted CA
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

// Main Application
void app_main(void) {
    // Initialize NVS
    nvs_flash_init();

    ESP_LOGI(TAG, "Initializing Wi-Fi...");
    wifi_init_ap_sta();

    // Wait until Wi-Fi is connected
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    ESP_LOGI(TAG, "Starting MQTT...");
    mqtt_app_start();

}
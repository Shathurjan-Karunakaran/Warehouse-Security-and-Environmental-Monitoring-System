#include "RFID.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>  // Added for memcpy

static const char *TAG = "RFID";

// Helper functions
static esp_err_t rfid_write_reg(rfid_config_t* config, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg << 1 | 0x80, value};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = data,
        .rx_buffer = NULL
    };
    return spi_device_transmit(config->spi_dev, &t);
}

static esp_err_t rfid_read_reg(rfid_config_t* config, uint8_t reg, uint8_t* value) {
    uint8_t data[2] = {reg << 1 | 0x81, 0};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = data,
        .rx_buffer = data
    };
    esp_err_t ret = spi_device_transmit(config->spi_dev, &t);
    if (ret == ESP_OK) {
        *value = data[1];
    }
    return ret;
}

esp_err_t rfid_init(rfid_config_t* config) {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    if (config->is_init) {
        ESP_LOGE(TAG, "RFID already initialized");
        return ESP_FAIL;
    }

    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->rst_gpio) | (1ULL << config->cs_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Set CS high and RST low
    gpio_set_level(config->cs_gpio, 1);
    gpio_set_level(config->rst_gpio, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set RST high
    gpio_set_level(config->rst_gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Soft reset
    rfid_write_reg(config, MFRC522_REG_COMM_IRQ_EN, 0x00);
    rfid_write_reg(config, MFRC522_REG_COMM_IRQ_EN, 0x80);
    rfid_write_reg(config, MFRC522_REG_COMMAND, MFRC522_CMD_SOFT_RESET);
    vTaskDelay(pdMS_TO_TICKS(2));

    // Configure antenna
    rfid_write_reg(config, MFRC522_REG_TX_MODE, 0x00);
    rfid_write_reg(config, MFRC522_REG_RX_MODE, 0x86);
    rfid_write_reg(config, MFRC522_REG_MOD_WIDTH, 0x26);
    rfid_write_reg(config, MFRC522_REG_TMODE, 0x80);
    rfid_write_reg(config, MFRC522_REG_TPRESCALER, 0x9A);
    rfid_write_reg(config, MFRC522_REG_TRELOAD_H, 0x86);
    rfid_write_reg(config, MFRC522_REG_TRELOAD_L, 0x10);
    rfid_write_reg(config, MFRC522_REG_TX_AUTO, 0x40);
    rfid_write_reg(config, MFRC522_REG_MODE, 0x3D);

    // Enable antenna
    uint8_t tx_control;
    rfid_read_reg(config, MFRC522_REG_TX_CONTROL, &tx_control);
    rfid_write_reg(config, MFRC522_REG_TX_CONTROL, tx_control | 0x03);

    config->is_init = true;
    return ESP_OK;
}

esp_err_t rfid_deinit(rfid_config_t* config) {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    if (!config->is_init) {
        ESP_LOGE(TAG, "RFID not initialized");
        return ESP_FAIL;
    }

    // Disable antenna
    uint8_t tx_control;
    rfid_read_reg(config, MFRC522_REG_TX_CONTROL, &tx_control);
    rfid_write_reg(config, MFRC522_REG_TX_CONTROL, tx_control & ~0x03);

    // Reset pins
    gpio_set_level(config->rst_gpio, 0);
    gpio_set_level(config->cs_gpio, 0);

    config->is_init = false;
    return ESP_OK;
}

esp_err_t rfid_is_card_present(rfid_config_t* config, bool* card_present) {
    uint8_t status;
    uint8_t buffer[2];
    uint8_t buffer_size = 2;

    // Send REQA command
    buffer[0] = MFRC522_CMD_TRANSCEIVE;
    buffer[1] = 0x26;  // REQA command

    spi_transaction_t t = {
        .length = buffer_size * 8,
        .tx_buffer = buffer,
        .rx_buffer = NULL
    };

    esp_err_t ret = spi_device_transmit(config->spi_dev, &t);
    if (ret != ESP_OK) {
        *card_present = false;
        return ret;
    }

    // Read status
    rfid_read_reg(config, MFRC522_REG_COMM_IRQ_EN, &status);
    *card_present = (status & 0x01) != 0;

    return ESP_OK;
}

esp_err_t rfid_read_card(rfid_config_t* config, rfid_card_t* card) {
    uint8_t status;
    uint8_t buffer[10];
    uint8_t buffer_size = 10;

    // Send REQA command
    buffer[0] = MFRC522_CMD_TRANSCEIVE;
    buffer[1] = 0x26;  // REQA command

    spi_transaction_t t = {
        .length = buffer_size * 8,
        .tx_buffer = buffer,
        .rx_buffer = buffer
    };

    esp_err_t ret = spi_device_transmit(config->spi_dev, &t);
    if (ret != ESP_OK) {
        return ret;
    }

    // Read status and UID
    rfid_read_reg(config, MFRC522_REG_COMM_IRQ_EN, &status);
    if ((status & 0x01) == 0) {
        return ESP_FAIL;
    }

    // Read UID length and data
    card->uid_len = buffer[0];
    memcpy(card->uid, &buffer[1], card->uid_len);

    // Determine card type (simplified)
    card->type = RFID_CARD_TYPE_MIFARE_CLASSIC_1K;  // Default type

    return ESP_OK;
}

esp_err_t rfid_write_card(rfid_config_t* config, rfid_card_t* card, uint8_t* data, size_t len) {
    // This is a simplified implementation
    // In a real application, you would need to implement proper authentication
    // and sector/trailer block handling
    ESP_LOGW(TAG, "Write operation not fully implemented");
    return ESP_ERR_NOT_SUPPORTED;
} 
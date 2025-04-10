#include "rc522.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

spi_device_handle_t spi;

#define MFRC522_REG_COMMAND 0x01
#define MFRC522_REG_FIFO_DATA 0x09
#define MFRC522_REG_FIFO_LEVEL 0x0A
#define MFRC522_REG_CONTROL 0x0C
#define MFRC522_REG_BIT_FRAMING 0x0D
#define MFRC522_REG_ERROR 0x06
#define MFRC522_REG_STATUS2 0x08
#define MFRC522_REG_COLL 0x0E
#define MFRC522_REG_MODE 0x11
#define MFRC522_REG_TX_MODE 0x12
#define MFRC522_REG_RX_MODE 0x13
#define MFRC522_REG_TX_CONTROL 0x14
#define MFRC522_REG_VERSION 0x37

#define PCD_IDLE 0x00
#define PCD_AUTHENT 0x0E
#define PCD_TRANSCEIVE 0x0C

#define PICC_REQIDL 0x26
#define PICC_ANTICOLL 0x93

void rc522_write_reg(uint8_t reg, uint8_t val) {
    uint8_t data[2] = { (reg << 1) & 0x7E, val };
    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = data,
    };
    spi_device_transmit(spi, &t);
}

uint8_t rc522_read_reg(uint8_t reg) {
    uint8_t tx[2] = { ((reg << 1) & 0x7E) | 0x80, 0x00 };
    uint8_t rx[2];
    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    spi_device_transmit(spi, &t);
    return rx[1];
}

void rc522_set_bit_mask(uint8_t reg, uint8_t mask) {
    rc522_write_reg(reg, rc522_read_reg(reg) | mask);
}

void rc522_clear_bit_mask(uint8_t reg, uint8_t mask) {
    rc522_write_reg(reg, rc522_read_reg(reg) & (~mask));
}

void rc522_antenna_on() {
    uint8_t val = rc522_read_reg(MFRC522_REG_TX_CONTROL);
    if (!(val & 0x03)) {
        rc522_set_bit_mask(MFRC522_REG_TX_CONTROL, 0x03);
    }
}

void rc522_reset() {
    rc522_write_reg(MFRC522_REG_COMMAND, PCD_IDLE);
    rc522_write_reg(MFRC522_REG_MODE, 0x3D);
    rc522_write_reg(MFRC522_REG_TX_MODE, 0x00);
    rc522_write_reg(MFRC522_REG_RX_MODE, 0x00);
    rc522_write_reg(MFRC522_REG_T_MODE, 0x80);
    rc522_write_reg(MFRC522_REG_T_PRESCALER, 0xA9);
    rc522_write_reg(MFRC522_REG_T_RELOAD_L, 0x03);
    rc522_write_reg(MFRC522_REG_T_RELOAD_H, 0xE8);
    rc522_write_reg(MFRC522_REG_TX_ASK, 0x40);
    rc522_write_reg(MFRC522_REG_MODE, 0x3D);
    rc522_antenna_on();
}

void rc522_init() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = RC522_MOSI_GPIO,
        .miso_io_num = RC522_MISO_GPIO,
        .sclk_io_num = RC522_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    spi_bus_initialize(RC522_SPI_HOST, &buscfg, SPI_DMA_DISABLED);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = RC522_CS_GPIO,
        .queue_size = 7,
    };
    spi_bus_add_device(RC522_SPI_HOST, &devcfg, &spi);

    rc522_reset();
}

bool rc522_read_card(uint8_t *uid, uint8_t *uid_length) {
    rc522_write_reg(MFRC522_REG_BIT_FRAMING, 0x07);
    rc522_write_reg(MFRC522_REG_COMMAND, PCD_TRANSCEIVE);
    rc522_write_reg(MFRC522_REG_FIFO_DATA, PICC_REQIDL);
    rc522_set_bit_mask(MFRC522_REG_BIT_FRAMING, 0x80);

    vTaskDelay(pdMS_TO_TICKS(50));

    if (!(rc522_read_reg(MFRC522_REG_FIFO_LEVEL))) return false;

    rc522_clear_bit_mask(MFRC522_REG_COLL, 0x80);
    rc522_write_reg(MFRC522_REG_COMMAND, PCD_TRANSCEIVE);
    rc522_write_reg(MFRC522_REG_FIFO_DATA, PICC_ANTICOLL);
    rc522_write_reg(MFRC522_REG_FIFO_DATA, 0x20);
    rc522_set_bit_mask(MFRC522_REG_BIT_FRAMING, 0x80);

    vTaskDelay(pdMS_TO_TICKS(50));

    *uid_length = rc522_read_reg(MFRC522_REG_FIFO_LEVEL);
    if (*uid_length < 4) return false;

    for (int i = 0; i < *uid_length; i++) {
        uid[i] = rc522_read_reg(MFRC522_REG_FIFO_DATA);
    }

    return true;
}

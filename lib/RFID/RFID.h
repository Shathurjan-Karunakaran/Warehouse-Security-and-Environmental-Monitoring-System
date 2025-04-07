#ifndef _RFID_MFRC522_H_
#define _RFID_MFRC522_H_

#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>  // Added for memcpy

// MFRC522 Register definitions
#define MFRC522_REG_RESET_IRQ    0x00
#define MFRC522_REG_COMM_IRQ_EN  0x02
#define MFRC522_REG_DIVL_EN      0x14
#define MFRC522_REG_COMM_TRN     0x04
#define MFRC522_REG_COMMAND      0x01
#define MFRC522_REG_BIT_FRAMING  0x0D
#define MFRC522_REG_COLL         0x0E
#define MFRC522_REG_MODE         0x11
#define MFRC522_REG_TX_MODE      0x12
#define MFRC522_REG_RX_MODE      0x13
#define MFRC522_REG_TX_CONTROL   0x14
#define MFRC522_REG_TX_AUTO      0x15
#define MFRC522_REG_TX_SEL       0x16
#define MFRC522_REG_RX_SEL       0x17
#define MFRC522_REG_RX_THRESHOLD 0x18
#define MFRC522_REG_DEMOD        0x19
#define MFRC522_REG_CRC_RESULT_M 0x21
#define MFRC522_REG_CRC_RESULT_L 0x22
#define MFRC522_REG_MOD_WIDTH    0x24
#define MFRC522_REG_RFCFG        0x26
#define MFRC522_REG_GSN          0x27
#define MFRC522_REG_CWGSP        0x28
#define MFRC522_REG_MODGSP       0x29
#define MFRC522_REG_TMODE        0x2A
#define MFRC522_REG_TPRESCALER   0x2B
#define MFRC522_REG_TRELOAD_H    0x2C
#define MFRC522_REG_TRELOAD_L    0x2D
#define MFRC522_REG_TCOUNTER_H   0x2E
#define MFRC522_REG_TCOUNTER_L   0x2F

// MFRC522 Commands
#define MFRC522_CMD_IDLE         0x00
#define MFRC522_CMD_MEM           0x01
#define MFRC522_CMD_GEN_RANDOM_ID 0x02
#define MFRC522_CMD_CALC_CRC      0x03
#define MFRC522_CMD_TRANSMIT      0x04
#define MFRC522_CMD_NO_CMD_CHANGE 0x07
#define MFRC522_CMD_RECEIVE       0x08
#define MFRC522_CMD_TRANSCEIVE    0x0C
#define MFRC522_CMD_MF_AUTHENT    0x0E
#define MFRC522_CMD_SOFT_RESET    0x0F

// RFID Card types
typedef enum {
    RFID_CARD_TYPE_UNKNOWN = 0,
    RFID_CARD_TYPE_MIFARE_MINI,
    RFID_CARD_TYPE_MIFARE_CLASSIC_1K,
    RFID_CARD_TYPE_MIFARE_CLASSIC_4K,
    RFID_CARD_TYPE_MIFARE_ULTRALIGHT,
    RFID_CARD_TYPE_MIFARE_PLUS,
    RFID_CARD_TYPE_MIFARE_DESFIRE
} rfid_card_type_t;

// RFID Card structure
typedef struct {
    uint8_t uid[10];           // Card UID
    uint8_t uid_len;           // Length of UID
    rfid_card_type_t type;     // Card type
} rfid_card_t;

// RFID configuration structure
typedef struct {
    spi_device_handle_t spi_dev;    // SPI device handle
    gpio_num_t rst_gpio;            // Reset GPIO pin
    gpio_num_t cs_gpio;             // Chip Select GPIO pin
    bool is_init;                   // Initialization flag
} rfid_config_t;

// Function declarations
esp_err_t rfid_init(rfid_config_t* config);
esp_err_t rfid_deinit(rfid_config_t* config);
esp_err_t rfid_is_card_present(rfid_config_t* config, bool* card_present);
esp_err_t rfid_read_card(rfid_config_t* config, rfid_card_t* card);
esp_err_t rfid_write_card(rfid_config_t* config, rfid_card_t* card, uint8_t* data, size_t len);

#endif // _RFID_MFRC522_H_ 
// rc522.h

#pragma once

#include "driver/spi_master.h"
#include <stdbool.h>
#include <stdint.h>

#define RC522_SPI_HOST HSPI_HOST
#define RC522_MOSI_GPIO 19
#define RC522_MISO_GPIO 21
#define RC522_SCLK_GPIO 18
#define RC522_CS_GPIO 5
// rc522.h (Add these above or below your function declarations)

// Page 0: Command and Status
#define MFRC522_REG_COMMAND 0x01
#define MFRC522_REG_COM_I_EN 0x02
#define MFRC522_REG_DIV_I_EN 0x03
#define MFRC522_REG_COM_IRQ 0x04
#define MFRC522_REG_DIV_IRQ 0x05
#define MFRC522_REG_ERROR 0x06
#define MFRC522_REG_STATUS1 0x07
#define MFRC522_REG_STATUS2 0x08
#define MFRC522_REG_FIFO_DATA 0x09
#define MFRC522_REG_FIFO_LEVEL 0x0A
#define MFRC522_REG_CONTROL 0x0C
#define MFRC522_REG_BIT_FRAMING 0x0D
#define MFRC522_REG_MODE 0x11

// Page 1: Command
#define MFRC522_REG_TX_MODE 0x12
#define MFRC522_REG_RX_MODE 0x13
#define MFRC522_REG_TX_CONTROL 0x14
#define MFRC522_REG_TX_AUTO 0x15
#define MFRC522_REG_TX_SELL 0x16
#define MFRC522_REG_RX_SELL 0x17
#define MFRC522_REG_RX_THRESHOLD 0x18
#define MFRC522_REG_TIMER_VALUE 0x1A

// Timer
#define MFRC522_REG_T_MODE  0x2A
#define MFRC522_REG_T_PRESCALER 0x2B
#define MFRC522_REG_T_RELOAD_H 0x2C
#define MFRC522_REG_T_RELOAD_L 0x2D

// Bit framing
#define MFRC522_REG_TX_ASK 0x15

// Reset pin is optional, so we don't need a register for that

void rc522_init(void);
bool rc522_read_card(uint8_t *uid, uint8_t *uid_length);

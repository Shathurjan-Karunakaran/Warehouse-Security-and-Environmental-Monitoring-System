#ifndef __MJD_HCSR501_H__
#define __MJD_HCSR501_H__

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration struct for HC-SR501 PIR sensor
 */
typedef struct {
    bool is_init;
    gpio_num_t data_gpio_num;
    SemaphoreHandle_t isr_semaphore;
} mjd_hcsr501_config_t;

/**
 * @brief Default config macro for the sensor
 */
#define MJD_HCSR501_CONFIG_DEFAULT() { \
    .is_init = false, \
    .data_gpio_num = GPIO_NUM_MAX, \
    .isr_semaphore = NULL \
}

/**
 * @brief Initialize the PIR sensor and configure GPIO/ISR
 *
 * @param ptr_param_config Pointer to configuration struct
 * @return esp_err_t
 */
esp_err_t mjd_hcsr501_init(mjd_hcsr501_config_t* ptr_param_config);

/**
 * @brief Deinitialize the PIR sensor and free resources
 *
 * @param ptr_param_config Pointer to configuration struct
 * @return esp_err_t
 */
esp_err_t mjd_hcsr501_deinit(mjd_hcsr501_config_t* ptr_param_config);

#ifdef __cplusplus
}
#endif

#endif /* __MJD_HCSR501_H__ */

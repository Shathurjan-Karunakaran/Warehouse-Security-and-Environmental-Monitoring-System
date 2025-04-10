/*
 * HC-SR501 PIR Human Infrared Sensor Module Including Lens
 */

 #include "mjd_hcsr501.h"
 #include "driver/gpio.h"
 #include "esp_log.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/semphr.h"
 #include "freertos/task.h"
 
 static const char TAG[] = "mjd_hcsr501";
 
 // Internal static reference to ISR semaphore
 static SemaphoreHandle_t _hcsr501_config_isr_semaphore = NULL;
 
 /*
  * Interrupt Service Routine
  */
 static void IRAM_ATTR sensor_gpio_isr_handler(void* arg) {
     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
     xSemaphoreGiveFromISR(_hcsr501_config_isr_semaphore, &xHigherPriorityTaskWoken);
     if (xHigherPriorityTaskWoken == pdTRUE) {
         portYIELD_FROM_ISR();
     }
 }
 
 esp_err_t mjd_hcsr501_init(mjd_hcsr501_config_t* param_ptr_config) {
     ESP_LOGD(TAG, "%s()", __func__);
     esp_err_t f_retval = ESP_OK;
 
     if (param_ptr_config->is_init == true) {
         ESP_LOGE(TAG, "ABORT. Already initialized.");
         return ESP_FAIL;
     }
 
     // Sensor stabilization period (5s)
     vTaskDelay(pdMS_TO_TICKS(5000));
 
     // Create binary semaphore
     param_ptr_config->isr_semaphore = xSemaphoreCreateBinary();
     if (param_ptr_config->isr_semaphore == NULL) {
         ESP_LOGE(TAG, "ABORT. Failed to create binary semaphore.");
         return ESP_FAIL;
     }
     _hcsr501_config_isr_semaphore = param_ptr_config->isr_semaphore;
 
     // Configure GPIO
     gpio_config_t io_conf = {
         .pin_bit_mask = (1ULL << param_ptr_config->data_gpio_num),
         .mode = GPIO_MODE_INPUT,
         .pull_up_en = GPIO_PULLUP_ENABLE,
         .pull_down_en = GPIO_PULLDOWN_DISABLE,
         .intr_type = GPIO_INTR_POSEDGE
     };
     f_retval = gpio_config(&io_conf);
     if (f_retval != ESP_OK) {
         ESP_LOGE(TAG, "gpio_config() failed: %s", esp_err_to_name(f_retval));
         goto cleanup;
     }
 
     // Install ISR service
     f_retval = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
     if (f_retval != ESP_OK && f_retval != ESP_ERR_INVALID_STATE) { // Service may already be installed
         ESP_LOGE(TAG, "gpio_install_isr_service() failed: %s", esp_err_to_name(f_retval));
         goto cleanup;
     }
 
     // Attach ISR handler
     f_retval = gpio_isr_handler_add(param_ptr_config->data_gpio_num, sensor_gpio_isr_handler, NULL);
     if (f_retval != ESP_OK) {
         ESP_LOGE(TAG, "gpio_isr_handler_add() failed: %s", esp_err_to_name(f_retval));
         goto cleanup;
     }
 
     // Set initialized flag
     param_ptr_config->is_init = true;
 
     return ESP_OK;
 
 cleanup:
     if (param_ptr_config->isr_semaphore) {
         vSemaphoreDelete(param_ptr_config->isr_semaphore);
         param_ptr_config->isr_semaphore = NULL;
         _hcsr501_config_isr_semaphore = NULL;
     }
     return f_retval;
 }
 
 esp_err_t mjd_hcsr501_deinit(mjd_hcsr501_config_t* param_ptr_config) {
     ESP_LOGD(TAG, "%s()", __func__);
     esp_err_t f_retval = ESP_OK;
 
     if (!param_ptr_config->is_init) {
         ESP_LOGE(TAG, "ABORT. Not initialized.");
         return ESP_FAIL;
     }
 
     // Remove ISR handler
     f_retval = gpio_isr_handler_remove(param_ptr_config->data_gpio_num);
     if (f_retval != ESP_OK) {
         ESP_LOGE(TAG, "gpio_isr_handler_remove() failed: %s", esp_err_to_name(f_retval));
         return f_retval;
     }
 
     // Uninstall ISR service
     gpio_uninstall_isr_service();
 
     // Delete semaphore
     vSemaphoreDelete(param_ptr_config->isr_semaphore);
     param_ptr_config->isr_semaphore = NULL;
     _hcsr501_config_isr_semaphore = NULL;
 
     param_ptr_config->is_init = false;
     return ESP_OK;
 }
 
#include "MQ2.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_timer.h"
#include "math.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Calibration curve constants (to be used for the specific gases)
const float LPGCurve[3] = {2.3, 0.21, 0.73};   // Example LPG curve
const float COCurve[3] = {2.3, 0.72, 0.92};    // Example CO curve
const float SmokeCurve[3] = {2.3, 0.53, 0.78}; // Example Smoke curve

static const char *TAG = "MQ2_SENSOR";  // Tag for logging

void mq2_init(MQ2* mq2, int pin) {
    mq2->pin = pin;
    mq2->Ro = -1.0;
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0); // Adjust the ADC channel accordingly
    ESP_LOGI(TAG, "MQ2 Sensor initialized.");
}

void mq2_begin(MQ2* mq2) {
    mq2->Ro = mq2_MQ_calibration(mq2);
    ESP_LOGI(TAG, "Ro: %.2f kohm", mq2->Ro);
}

void mq2_close(MQ2* mq2) {
    mq2->Ro = -1.0;
    for (int i = 0; i < 3; i++) {
        mq2->values[i] = 0.0;
    }
}

bool mq2_check_calibration(MQ2* mq2) {
    if (mq2->Ro < 0.0) {
        ESP_LOGE(TAG, "Device not calibrated, call mq2_begin before reading any value.");
        return false;
    }
    return true;
}

float* mq2_read(MQ2* mq2, bool print) {
    if (!mq2_check_calibration(mq2)) return NULL;

    mq2->values[0] = mq2_MQ_get_percentage(mq2, (float*)LPGCurve);
    mq2->values[1] = mq2_MQ_get_percentage(mq2, (float*)COCurve);
    mq2->values[2] = mq2_MQ_get_percentage(mq2, (float*)SmokeCurve);

    mq2->lastReadTime = esp_timer_get_time() / 1000; // Get current time in milliseconds

    if (print) {
        ESP_LOGI(TAG, "%lu ms - LPG: %.5f ppm, CO: %.5f ppm, SMOKE: %.5f ppm", 
                 mq2->lastReadTime, mq2->values[0], mq2->values[1], mq2->values[2]);
    }

    return mq2->values;
}

float mq2_read_LPG(MQ2* mq2) {
    if (!mq2_check_calibration(mq2)) return 0.0;

    if (esp_timer_get_time() / 1000 < (mq2->lastReadTime + READ_DELAY) && mq2->values[0] > 0)
        return mq2->values[0];
    else
        return (mq2->values[0] = mq2_MQ_get_percentage(mq2, LPGCurve));
}

float mq2_read_CO(MQ2* mq2) {
    if (!mq2_check_calibration(mq2)) return 0.0;

    if (esp_timer_get_time() / 1000 < (mq2->lastReadTime + READ_DELAY) && mq2->values[1] > 0)
        return mq2->values[1];
    else
        return (mq2->values[1] = mq2_MQ_get_percentage(mq2, COCurve));
}

float mq2_read_smoke(MQ2* mq2) {
    if (!mq2_check_calibration(mq2)) return 0.0;

    if (esp_timer_get_time() / 1000 < (mq2->lastReadTime + READ_DELAY) && mq2->values[2] > 0)
        return mq2->values[2];
    else
        return (mq2->values[2] = mq2_MQ_get_percentage(mq2, SmokeCurve));
}

float mq2_MQ_resistance_calculation(MQ2* mq2, int raw_adc) {
    float flt_adc = (float) raw_adc;
    return RL_VALUE * (1023.0 - flt_adc) / flt_adc;
}

float mq2_MQ_calibration(MQ2* mq2) {
    float val = 0.0;

    // Take multiple samples for calibration
    for (int i = 0; i < CALIBRATION_SAMPLE_TIMES; i++) {
        val += mq2_MQ_resistance_calculation(mq2, adc1_get_raw(ADC1_CHANNEL_0));
        vTaskDelay(pdMS_TO_TICKS(CALIBRATION_SAMPLE_INTERVAL));  // ESP-IDF delay
    }

    val = val / ((float) CALIBRATION_SAMPLE_TIMES);
    val = val / RO_CLEAN_AIR_FACTOR;

    return val; 
}

float mq2_MQ_read(MQ2* mq2) {
    float rs = 0.0;

    // Read multiple samples
    for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
        rs += mq2_MQ_resistance_calculation(mq2, adc1_get_raw(ADC1_CHANNEL_0));
        vTaskDelay(pdMS_TO_TICKS(READ_SAMPLE_INTERVAL)); // ESP-IDF delay
    }

    return rs / ((float) READ_SAMPLE_TIMES);  // Return the average value
}

float mq2_MQ_get_percentage(MQ2* mq2, const float *pcurve) {
    float rs_ro_ratio = mq2_MQ_read(mq2) / mq2->Ro;
    return pow(10.0, ((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0]);
}

#include "MQ2.h"
#include "esp_log.h"
// Removed #include "driver/gpio.h" -> not directly used by ADC logic
// Removed #include "driver/adc.h" -> Replaced by new headers
#include "esp_adc/adc_oneshot.h" // New ADC driver
#include "hal/adc_types.h"       // For ADC enums

#include "esp_timer.h"
#include "math.h"        // For pow, log10, fabs, isinf, isnan
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- Calibration Curve Constants ---
// WARNING: These are EXAMPLE values. For accurate PPM readings, you MUST
// calibrate your specific sensor or use curves derived from its datasheet graph.
// Format: {log10(Reference PPM), log10(Rs/Ro at Reference PPM), slope of the log-log line}
const float LPGCurve[3]  = {3.0, 0.30, -0.45}; // EXAMPLE for LPG
const float COCurve[3]   = {3.6, 0.65, -0.38}; // EXAMPLE for CO (using 4000ppm ref)
const float SmokeCurve[3]= {3.0, 0.45, -0.40}; // EXAMPLE for Smoke (using 1000ppm ref)


static const char *TAG = "MQ2_SENSOR";  // Tag for logging

// --- Initialization and Setup ---

esp_err_t mq2_init(MQ2* mq2, adc_unit_t adc_unit, adc_channel_t adc_channel, adc_atten_t adc_atten) {
    if (mq2 == NULL) {
        ESP_LOGE(TAG, "mq2_init: NULL pointer passed for mq2 structure.");
        return ESP_ERR_INVALID_ARG;
    }
     if (adc_unit != ADC_UNIT_1) {
         // ADC2 is not recommended with WiFi
         ESP_LOGE(TAG, "mq2_init: Only ADC_UNIT_1 is supported due to WiFi compatibility.");
         return ESP_ERR_INVALID_ARG;
     }

    mq2->adc_channel = adc_channel;
    mq2->adc_atten = adc_atten;
    mq2->Ro = -1.0; // Initialize Ro to invalid state
    mq2->rl_value = RL_VALUE; // Store configured Load Resistor value
    mq2->ro_clean_air_factor = RO_CLEAN_AIR_FACTOR; // Store configured clean air factor
    mq2->lastReadTime = 0;
    mq2->adc_handle = NULL; // Initialize handle to NULL
    mq2->adc_initialized = false;
    for(int i=0; i<3; ++i) mq2->values[i] = NAN; // Clear initial values using NAN

    // --- ADC Oneshot Init ---
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = adc_unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE, // ULP mode not needed for this use case
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &mq2->adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // --- ADC Oneshot Channel Config ---
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12, // Use 12-bit resolution
        .atten = mq2->adc_atten,
    };
    ret = adc_oneshot_config_channel(mq2->adc_handle, mq2->adc_channel, &config);
     if (ret != ESP_OK) {
         ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
         // Clean up the unit handle if channel config fails
         adc_oneshot_del_unit(mq2->adc_handle);
         mq2->adc_handle = NULL;
         return ret;
     }

    mq2->adc_initialized = true;
    ESP_LOGI(TAG, "MQ2 Sensor ADC Initialized: Unit %d, Channel %d, Attenuation %d", adc_unit, mq2->adc_channel, mq2->adc_atten);
    ESP_LOGI(TAG, "Using RL_VALUE: %.2f kOhm, RO_CLEAN_AIR_FACTOR: %.2f", mq2->rl_value, mq2->ro_clean_air_factor);
    ESP_LOGW(TAG, "Ensure sensor is pre-heated adequately before calibration (mq2_begin)!");
    return ESP_OK;
}

// Deinitialization function
void mq2_deinit(MQ2* mq2) {
     if (mq2 != NULL && mq2->adc_initialized && mq2->adc_handle != NULL) {
         esp_err_t ret = adc_oneshot_del_unit(mq2->adc_handle);
          if (ret != ESP_OK) {
               ESP_LOGE(TAG, "adc_oneshot_del_unit failed: %s", esp_err_to_name(ret));
           } else {
               ESP_LOGI(TAG, "MQ2 ADC unit deinitialized.");
           }
         mq2->adc_handle = NULL;
         mq2->adc_initialized = false;
         mq2->Ro = -1.0; // Reset calibration state
     }
}


bool mq2_begin(MQ2* mq2) {
     if (mq2 == NULL || !mq2->adc_initialized) {
        ESP_LOGE(TAG, "mq2_begin: Sensor not initialized properly.");
        return false;
    }
    ESP_LOGI(TAG, "Starting MQ2 calibration... (Ensure sensor is in clean air and pre-heated)");
    mq2->Ro = mq2_MQ_calibration(mq2); // This calls mq2_MQ_read_adc internally

    if (mq2->Ro > 0) {
         ESP_LOGI(TAG, "Calibration successful. Ro = %.3f kOhm", mq2->Ro);
         return true;
    } else {
         // Ro will be < 0 if calibration failed (returned -1.0)
         ESP_LOGE(TAG, "Calibration failed. Ro calculation resulted in %.3f.", mq2->Ro);
         ESP_LOGE(TAG, "Check: Sensor connections, heating, clean air, RL_VALUE (%.2f kOhm), RO_CLEAN_AIR_FACTOR (%.2f)", mq2->rl_value, mq2->ro_clean_air_factor);
         return false;
    }
}


bool mq2_check_calibration(MQ2* mq2) {
    // Check for null pointer, ADC init status, and valid Ro (> 0)
    if (mq2 == NULL || !mq2->adc_initialized || mq2->Ro <= 0.0) {
        return false;
    }
    return true;
}

// --- Reading Functions ---

float* mq2_read(MQ2* mq2, bool print) {
    if (!mq2_check_calibration(mq2)) {
        // Return NULL if not calibrated
        return NULL;
    }

    // Get current sensor resistance (Rs) using the new ADC read function
    float rs = mq2_MQ_read_adc(mq2); // Reads average Rs, returns < 0 on error
    if (rs < 0) {
        ESP_LOGE(TAG, "Failed to read valid sensor resistance (Rs=%.3f).", rs);
        // Set stored values to NAN or negative to indicate read error
        for(int i=0; i<3; ++i) mq2->values[i] = -1.0; // Or NAN
        return NULL; // Indicate read failure
    }

    // Calculate Rs/Ro ratio
    float ratio = rs / mq2->Ro; // Ro is guaranteed > 0 here due to mq2_check_calibration

    // Calculate PPM for each gas using the Rs/Ro ratio and respective curves
    // mq2_MQ_get_percentage will return < 0 if calculation fails for any reason
    mq2->values[0] = mq2_MQ_get_percentage(ratio, LPGCurve);
    mq2->values[1] = mq2_MQ_get_percentage(ratio, COCurve);
    mq2->values[2] = mq2_MQ_get_percentage(ratio, SmokeCurve);

    // Update timestamp
    mq2->lastReadTime = esp_timer_get_time() / 1000ULL; // Get current time in milliseconds

    if (print) {
        // Check for calculation errors before printing PPM values
        char lpg_str[15], co_str[15], smoke_str[15];
        snprintf(lpg_str, sizeof(lpg_str), mq2->values[0] < 0 ? "ERR" : "%.3f", mq2->values[0]);
        snprintf(co_str, sizeof(co_str), mq2->values[1] < 0 ? "ERR" : "%.3f", mq2->values[1]);
        snprintf(smoke_str, sizeof(smoke_str), mq2->values[2] < 0 ? "ERR" : "%.3f", mq2->values[2]);

        ESP_LOGI(TAG, "%llu ms - LPG: %s ppm, CO: %s ppm, SMOKE: %s ppm (Rs=%.3fk, Ro=%.3fk, Ratio=%.3f)",
                 mq2->lastReadTime, lpg_str, co_str, smoke_str, rs, mq2->Ro, ratio);
    }

    return mq2->values; // Return pointer to the results array
}

// Helper for individual gas reads with caching
static float mq2_read_single_gas(MQ2* mq2, int gas_index, const float *curve) {
    // Check calibration first - return NAN if not calibrated
     if (!mq2_check_calibration(mq2)) return NAN;

    uint64_t current_time_ms = esp_timer_get_time() / 1000ULL;

    // Use cached value if the specific gas value is valid (not NAN, >=0) and recent
    if (current_time_ms < (mq2->lastReadTime + READ_DELAY) && !isnan(mq2->values[gas_index]) && mq2->values[gas_index] >= 0) {
        return mq2->values[gas_index];
    } else {
        // Cache miss or expired: Need to re-read sensor resistance
        float rs = mq2_MQ_read_adc(mq2); // Use new ADC read function, returns < 0 on error
        if (rs < 0) {
            mq2->values[gas_index] = -1.0; // Store error indicator
            return -1.0; // Return error indicator
        }
        float ratio = rs / mq2->Ro; // Ro is valid here
        // Calculate and store the new value (mq2_MQ_get_percentage returns < 0 on error)
        mq2->values[gas_index] = mq2_MQ_get_percentage(ratio, curve);
        // Note: This single read doesn't update lastReadTime globally, only mq2_read does.
        return mq2->values[gas_index];
    }
}


float mq2_read_LPG(MQ2* mq2) {
    return mq2_read_single_gas(mq2, 0, LPGCurve);
}

float mq2_read_CO(MQ2* mq2) {
     return mq2_read_single_gas(mq2, 1, COCurve);
}

float mq2_read_smoke(MQ2* mq2) {
     return mq2_read_single_gas(mq2, 2, SmokeCurve);
}


// --- Core Calculation Functions ---

// Resistance calculation function remains largely the same conceptually
float mq2_MQ_resistance_calculation(MQ2* mq2, int raw_adc) {
    if (mq2 == NULL) return -1.0; // Safety check

    if (raw_adc <= 0) {
        // ADC value can be 0 if input voltage is 0V. Resistance would be infinite.
        // Treat as error for simplicity. Could return INFINITY if needed.
        ESP_LOGD(TAG, "ADC value is zero or negative (%d).", raw_adc); // Use Debug level
        return -2.0; // Use a different negative value
    }
    if (raw_adc >= 4095) {
         // ADC saturated (input voltage likely >= Vref used for attenuation).
         // Sensor resistance is very low (approaching zero). Rs=0 is valid.
         ESP_LOGD(TAG, "ADC value saturated at %d.", raw_adc); // Use Debug level
         // Allow calculation to proceed, which should result in Rs near 0.
    }

    // Formula: Rs = RL * (ADC_MAX_VALUE - raw_adc) / raw_adc
    // ADC_MAX_VALUE for 12-bit is 4095.0
    float flt_adc = (float) raw_adc;
    // Check divisor just in case, although handled by raw_adc <= 0 check
    if (flt_adc == 0) return -2.0;

    return mq2->rl_value * (4095.0 - flt_adc) / flt_adc;
}

// Calibration function - uses the new ADC read primitive
float mq2_MQ_calibration(MQ2* mq2) {
     if (mq2 == NULL || !mq2->adc_initialized) return -1.0;

    float rs_sum = 0.0;
    int valid_samples = 0;
    int adc_raw = 0; // Variable to store ADC reading result
    ESP_LOGI(TAG, "Calibration: Reading %d samples with %dms interval...", CALIBRATION_SAMPLE_TIMES, CALIBRATION_SAMPLE_INTERVAL);

    for (int i = 0; i < CALIBRATION_SAMPLE_TIMES; i++) {
        // Use the new oneshot read function
        esp_err_t ret = adc_oneshot_read(mq2->adc_handle, mq2->adc_channel, &adc_raw);

        if (ret == ESP_OK) { // ADC read successful
             float rs_val = mq2_MQ_resistance_calculation(mq2, adc_raw);
             if (rs_val >= 0) { // Check if resistance calculation is valid
                 rs_sum += rs_val;
                 valid_samples++;
             } else {
                 ESP_LOGW(TAG, "Calibration sample %d: Rs calculation invalid (ADC: %d, Rs calc: %.3f)", i + 1, adc_raw, rs_val);
             }
        } else {
            ESP_LOGW(TAG, "Calibration sample %d: adc_oneshot_read failed: %s", i + 1, esp_err_to_name(ret));
             // Optionally add a small delay here if ADC reads fail consecutively
             vTaskDelay(pdMS_TO_TICKS(5));
        }

        // Delay between samples
        vTaskDelay(pdMS_TO_TICKS(CALIBRATION_SAMPLE_INTERVAL));
    }

    if (valid_samples == 0) {
        ESP_LOGE(TAG, "Calibration failed: No valid resistance samples obtained.");
        return -1.0; // Indicate calibration failure
    }

    float rs_avg = rs_sum / ((float) valid_samples);
    // Ensure clean air factor is positive to avoid division by zero or invalid Ro
    if (mq2->ro_clean_air_factor <= 0) {
        ESP_LOGE(TAG, "Calibration failed: Invalid RO_CLEAN_AIR_FACTOR (%.2f)", mq2->ro_clean_air_factor);
        return -1.0;
    }
    float ro_calc = rs_avg / mq2->ro_clean_air_factor; // Calculate Ro using the clean air factor

    ESP_LOGI(TAG, "Calibration: Average Rs in clean air = %.3f kOhm (%d valid samples)", rs_avg, valid_samples);
    return ro_calc; // Return calculated Ro
}


// Internal function to read average sensor resistance using new ADC driver
float mq2_MQ_read_adc(MQ2* mq2) {
    if (mq2 == NULL || !mq2->adc_initialized) return -1.0;

    float rs_sum = 0.0;
    int valid_samples = 0;
    int adc_raw = 0;

    for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
         esp_err_t ret = adc_oneshot_read(mq2->adc_handle, mq2->adc_channel, &adc_raw);
         if (ret == ESP_OK) {
            float rs_val = mq2_MQ_resistance_calculation(mq2, adc_raw);
            if (rs_val >= 0) { // Check if calculation was valid and non-negative
                 rs_sum += rs_val;
                 valid_samples++;
            }
            // else: resistance calculation failed, warning already logged by it
         } else {
              // Read failed - don't log excessively here during normal operation
               // Consider counting failures and logging only if persistent
         }


        // Only delay if not the last sample
        if (i < READ_SAMPLE_TIMES - 1) {
            vTaskDelay(pdMS_TO_TICKS(READ_SAMPLE_INTERVAL));
        }
    }

     if (valid_samples == 0) {
        ESP_LOGW(TAG, "MQ_read_adc: Failed to obtain any valid resistance samples.");
        return -1.0; // Indicate read failure
    }

    // Return the average sensor resistance Rs
    return rs_sum / ((float) valid_samples);
}


// --- PPM Calculation ---
// (This function doesn't depend on ADC driver details, no changes needed here
//  except validation logic as implemented previously)
float mq2_MQ_get_percentage(float rs_ro_ratio, const float *pcurve) {
    // pcurve format: {log10(Reference PPM), log10(Rs/Ro at Reference PPM), slope}

    // --- Input Validation ---
    if (pcurve == NULL) {
         ESP_LOGE(TAG, "MQ_get_percentage: Curve data pointer is NULL.");
         return -1.0; // Error: Missing curve data
    }
    if (fabs((double)pcurve[2]) < 1e-9) { // Check for zero slope
        ESP_LOGE(TAG, "MQ_get_percentage: Curve slope is effectively zero (pcurve[2]=%.4f).", pcurve[2]);
        return -2.0; // Error: Division by zero
    }
    if (rs_ro_ratio <= 0) { // Check for invalid ratio
        ESP_LOGD(TAG, "MQ_get_percentage: Invalid input ratio (<= 0): %.4f", rs_ro_ratio);
        return -3.0; // Error: Log of non-positive
    }

    // --- Calculation ---
    double log10_ratio = log10((double)rs_ro_ratio);
    double log10_ppm = (log10_ratio - (double)pcurve[1]) / (double)pcurve[2] + (double)pcurve[0];
    double ppm_double = pow(10.0, log10_ppm);

    // --- Result Validation ---
     if (isinf(ppm_double) || isnan(ppm_double)) {
        ESP_LOGW(TAG, "MQ_get_percentage: Calculation resulted in Infinity or NaN (Ratio=%.4f)", rs_ro_ratio);
        return -4.0; // Error: Calculation error
    }

    return (float)ppm_double; // Return calculated PPM
}
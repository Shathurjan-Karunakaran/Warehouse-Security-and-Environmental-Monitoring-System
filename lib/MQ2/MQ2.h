#ifndef MQ2_H
#define MQ2_H

#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "esp_timer.h" // For uint64_t
#include <stdbool.h>   // For bool type
#include <math.h>      // For NAN

// --- Configuration Constants (ADJUST THESE) ---

// Define the load resistor value (RL) in kiloOhms (kΩ).
// Common values are 5kΩ or 10kΩ. Check your specific MQ2 module's schematic.
#define RL_VALUE (5.0)

// Define the ratio Rs/Ro for clean air.
// This value is found in the sensor datasheet (typically around 9.8 for MQ-2).
// Fine-tune this value by reading Rs in known clean air for better calibration.
#define RO_CLEAN_AIR_FACTOR (9.83)

// Define the number of samples and interval for calibration phase.
#define CALIBRATION_SAMPLE_TIMES (50)
#define CALIBRATION_SAMPLE_INTERVAL (500) // milliseconds

// Define the number of samples and interval for reading phase.
#define READ_SAMPLE_TIMES (5)
#define READ_SAMPLE_INTERVAL (50) // milliseconds

// Define the delay (in ms) for cached readings in individual read functions.
#define READ_DELAY (100) // ms - Only re-read if last read was longer ago than this

// --- Sensor Data Structure ---

typedef struct {
    adc1_channel_t adc_channel;  // ADC channel the sensor's AO pin is connected to
    adc_atten_t adc_atten;       // ADC attenuation setting used for the channel
    adc_oneshot_unit_handle_t adc_handle; // Handle for the ADC oneshot unit
    float Ro;                    // Sensor resistance in clean air (calibrated value in kOhm). Should be > 0 after calibration.
    float values[3];             // Stores last read concentrations [LPG, CO, SMOKE] in PPM. Value < 0 might indicate read/calc error.
    uint64_t lastReadTime;       // Timestamp of the last full read (in milliseconds)
    float rl_value;              // Stored Load Resistor value (kOhm) used for calculations
    float ro_clean_air_factor;   // Stored Clean Air Factor (Rs/Ro in clean air) used for calibration
    bool adc_initialized;
} MQ2;

// --- Function Prototypes ---

/**
 * @brief Initializes the MQ2 sensor structure and configures the ADC.
 * @note Ensure the ESP32 pin connected to the sensor's AO output corresponds
 *       to the provided adc_channel.
 *
 * @param mq2 Pointer to the MQ2 structure.
 * @param adc_channel The ADC1 channel the sensor is connected to (e.g., ADC1_CHANNEL_6 for GPIO34).
 * @param adc_atten The ADC attenuation to use (e.g., ADC_ATTEN_DB_11 for ~0-3.3V range is recommended).
 */
esp_err_t mq2_init(MQ2* mq2, adc_unit_t adc_unit, adc_channel_t adc_channel, adc_atten_t adc_atten);

/**
 * @brief Performs calibration to determine Ro (resistance in clean air).
 * @warning Sensor MUST be in clean air and adequately pre-heated (check datasheet,
 *          often minutes to hours) before calling this function for accurate Ro.
 * @param mq2 Pointer to the MQ2 structure. Populates mq2->Ro.
 */
bool mq2_begin(MQ2* mq2);

/**
 * @brief Resets the calibration value (Ro to -1.0) and clears stored readings.
 *
 * @param mq2 Pointer to the MQ2 structure.
 */
void mq2_deinit(MQ2* mq2);

/**
 * @brief Checks if the sensor has been successfully calibrated (Ro value is positive).
 *
 * @param mq2 Pointer to the MQ2 structure.
 * @return true if calibrated (Ro > 0), false otherwise.
 */
bool mq2_check_calibration(MQ2* mq2);

/**
 * @brief Reads all gas concentrations (LPG, CO, Smoke), updates the values array,
 *        and returns a pointer to it.
 *
 * @param mq2 Pointer to the MQ2 structure.
 * @param print If true, prints the readings to the console via ESP_LOGI.
 * @return Pointer to the internal float array holding [LPG, CO, Smoke] PPM values,
 *         or NULL if the sensor is not calibrated or a read error occurred.
 *         Individual values in the array might be < 0 if calculation failed for that gas.
 */
float* mq2_read(MQ2* mq2, bool print);

/**
 * @brief Reads the LPG concentration in PPM. Uses cached value if recent.
 *
 * @param mq2 Pointer to the MQ2 structure.
 * @return LPG concentration in PPM. Returns 0.0 if not calibrated.
 *         Returns a negative value (e.g., -1.0) if a read/calculation error occurs.
 */
float mq2_read_LPG(MQ2* mq2);

/**
 * @brief Reads the CO concentration in PPM. Uses cached value if recent.
 *
 * @param mq2 Pointer to the MQ2 structure.
 * @return CO concentration in PPM. Returns 0.0 if not calibrated.
 *         Returns a negative value (e.g., -1.0) if a read/calculation error occurs.
 */
float mq2_read_CO(MQ2* mq2);

/**
 * @brief Reads the Smoke concentration in PPM. Uses cached value if recent.
 *
 * @param mq2 Pointer to the MQ2 structure.
 * @return Smoke concentration in PPM. Returns 0.0 if not calibrated.
 *         Returns a negative value (e.g., -1.0) if a read/calculation error occurs.
 */
float mq2_read_smoke(MQ2* mq2);

// --- Internal Helper Functions (Generally not called directly by user) ---

/**
 * @brief Calculates the sensor's resistance (Rs) based on the raw ADC reading.
 *
 * @param mq2 Pointer to the MQ2 structure (used to get rl_value).
 * @param raw_adc The raw value read from the ADC (0-4095 for 12-bit).
 * @return Calculated sensor resistance (Rs) in kiloOhms (based on RL_VALUE unit).
 *         Returns a negative value (< 0) if raw_adc is invalid (e.g., <= 0).
 */
float mq2_MQ_resistance_calculation(MQ2* mq2, int raw_adc);

/**
 * @brief Performs the calibration process by averaging resistance readings in clean air.
 *        (Called internally by mq2_begin).
 *
 * @param mq2 Pointer to the MQ2 structure.
 * @return Calculated baseline resistance (Ro) in kiloOhms.
 *         Returns a negative value (< 0) if calibration fails (e.g., no valid ADC readings).
 */
float mq2_MQ_calibration(MQ2* mq2);

/**
 * @brief Reads the current sensor resistance (Rs) by averaging multiple samples.
 *
 * @param mq2 Pointer to the MQ2 structure.
 * @return Average sensor resistance (Rs) in kiloOhms.
 *         Returns a negative value (< 0) if reading fails (e.g., no valid ADC readings).
 */
float mq2_MQ_read_adc(MQ2* mq2);

/**
 * @brief Calculates the gas concentration in PPM based on Rs/Ro ratio and a curve.
 * @note The curve parameters are crucial for accuracy and often need specific calibration.
 *
 * @param rs_ro_ratio The calculated ratio of current sensor resistance (Rs) to baseline (Ro).
 * @param pcurve Pointer to the float array defining the gas curve
 *               [log10(ppm_ref), log10(rsro_ref), slope].
 * @return Calculated gas concentration in PPM.
 *         Returns a negative value (< 0) if input ratio is invalid, curve is NULL,
 *         slope is zero, or calculation results in error (inf/nan).
 */
float mq2_MQ_get_percentage(float rs_ro_ratio, const float *pcurve);

#endif // MQ2_H
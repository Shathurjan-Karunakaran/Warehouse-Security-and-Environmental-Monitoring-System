#ifndef __MQ2_H__
#define __MQ2_H__

#include "esp_adc/adc_oneshot.h"  // For single ADC readings
#include "esp_adc/adc_continuous.h"  // For continuous ADC readings
#include "esp_log.h"

// MQ2 sensor configuration
#define MQ2_PIN 34  // Example GPIO pin for the sensor (use correct pin)
#define RL_VALUE 10.0 // Load resistor value in kilo-ohms
#define RO_CLEAN_AIR_FACTOR 9.0 // Calibration factor for clean air
#define CALIBRATION_SAMPLE_TIMES 5
#define CALIBRATION_SAMPLE_INTERVAL 50
#define READ_SAMPLE_TIMES 5
#define READ_SAMPLE_INTERVAL 50
#define READ_DELAY 5000  // Delay time between reads (5 seconds)

// Calibration curve arrays for different gases (LPG, CO, Smoke)
extern const float LPGCurve[3];
extern const float COCurve[3];
extern const float SmokeCurve[3];

// MQ2 Class Declaration
typedef struct {
    int pin;                 // Sensor pin
    float Ro;                // Sensor resistance in clean air
    float values[3];         // Values for LPG, CO, Smoke
    uint32_t lastReadTime;  // Time of last sensor read
} MQ2;

void mq2_init(MQ2* mq2, int pin);
void mq2_begin(MQ2* mq2);
void mq2_close(MQ2* mq2);
bool mq2_check_calibration(MQ2* mq2);
float* mq2_read(MQ2* mq2, bool print);
float mq2_read_LPG(MQ2* mq2);
float mq2_read_CO(MQ2* mq2);
float mq2_read_smoke(MQ2* mq2);
float mq2_MQ_resistance_calculation(MQ2* mq2, int raw_adc);
float mq2_MQ_calibration(MQ2* mq2);
float mq2_MQ_read(MQ2* mq2);
float mq2_MQ_get_percentage(MQ2* mq2, const float *pcurve);

#endif // __MQ2_H__

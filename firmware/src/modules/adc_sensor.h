#ifndef BLE_SENSOR_LOGGER_ADC_SENSOR_H_
#define BLE_SENSOR_LOGGER_ADC_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

int adc_sensor_init(void);
void adc_sensor_start(void);
void adc_sensor_stop(void);
void adc_sensor_get_latest(int16_t *raw, uint16_t *millivolts, bool *valid);

#endif

#ifndef BLE_SENSOR_LOGGER_SENSOR_DUMMY_H_
#define BLE_SENSOR_LOGGER_SENSOR_DUMMY_H_

#include <stdint.h>

int sensor_dummy_init(void);
void sensor_dummy_start(void);
void sensor_dummy_stop(void);
void sensor_dummy_reset_sequence(void);
void sensor_dummy_set_interval(uint16_t interval_ms);

#endif

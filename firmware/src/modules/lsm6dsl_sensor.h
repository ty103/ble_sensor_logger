#ifndef BLE_SENSOR_LOGGER_LSM6DSL_SENSOR_H_
#define BLE_SENSOR_LOGGER_LSM6DSL_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

int lsm6dsl_sensor_init(void);
bool lsm6dsl_sensor_is_available(void);
uint16_t lsm6dsl_sensor_status_error(void);
void lsm6dsl_sensor_start(void);
void lsm6dsl_sensor_stop(void);
void lsm6dsl_sensor_reset_sequence(void);

#endif

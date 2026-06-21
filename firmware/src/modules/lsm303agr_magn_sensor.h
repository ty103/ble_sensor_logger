#ifndef BLE_SENSOR_LOGGER_LSM303AGR_MAGN_SENSOR_H_
#define BLE_SENSOR_LOGGER_LSM303AGR_MAGN_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

int lsm303agr_magn_sensor_init(void);
void lsm303agr_magn_sensor_start(void);
void lsm303agr_magn_sensor_stop(void);
void lsm303agr_magn_sensor_reset_sequence(void);
bool lsm303agr_magn_sensor_is_available(void);
uint16_t lsm303agr_magn_sensor_status_error(void);

#endif

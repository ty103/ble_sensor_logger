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
void lsm6dsl_sensor_set_complementary_alpha_permille(uint16_t alpha_permille);
void lsm6dsl_sensor_set_mahony_kp_milli(uint16_t kp_milli);
void lsm6dsl_sensor_set_mahony_ki_milli(uint16_t ki_milli);
void lsm6dsl_sensor_set_iir_cutoff_millihz(uint16_t cutoff_millihz);

#endif

#ifndef BLE_SENSOR_LOGGER_LPS22HB_SENSOR_H_
#define BLE_SENSOR_LOGGER_LPS22HB_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

int lps22hb_sensor_init(void);
void lps22hb_sensor_start(void);
void lps22hb_sensor_stop(void);
void lps22hb_sensor_reset_sequence(void);
bool lps22hb_sensor_is_available(void);
uint16_t lps22hb_sensor_status_error(void);

#endif

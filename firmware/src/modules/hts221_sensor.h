#ifndef BLE_SENSOR_LOGGER_HTS221_SENSOR_H_
#define BLE_SENSOR_LOGGER_HTS221_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

int hts221_sensor_init(void);
void hts221_sensor_start(void);
void hts221_sensor_stop(void);
void hts221_sensor_reset_sequence(void);
bool hts221_sensor_is_available(void);
uint16_t hts221_sensor_status_error(void);

#endif

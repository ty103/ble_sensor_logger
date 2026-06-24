#ifndef BLE_SENSOR_LOGGER_BLE_SERVICE_H_
#define BLE_SENSOR_LOGGER_BLE_SERVICE_H_

#include "protocol.h"

int ble_service_init(void);
int ble_service_enqueue_sample(const struct bsl_sensor_data *sample);
int ble_service_flush_samples(void);
void ble_service_set_status(const struct bsl_status *status);
void ble_service_set_config(const struct bsl_config *config);

#endif

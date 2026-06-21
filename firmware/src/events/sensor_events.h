#ifndef BLE_SENSOR_LOGGER_SENSOR_EVENTS_H_
#define BLE_SENSOR_LOGGER_SENSOR_EVENTS_H_

#include <app_event_manager.h>
#include "protocol.h"

struct sensor_sample_event {
	struct app_event_header header;
	struct bsl_sensor_data sample;
};

APP_EVENT_TYPE_DECLARE(sensor_sample_event);

#endif

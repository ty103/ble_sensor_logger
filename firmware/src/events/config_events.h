#ifndef BLE_SENSOR_LOGGER_CONFIG_EVENTS_H_
#define BLE_SENSOR_LOGGER_CONFIG_EVENTS_H_

#include <app_event_manager.h>
#include "protocol.h"

struct config_update_event {
	struct app_event_header header;
	struct bsl_config config;
};

APP_EVENT_TYPE_DECLARE(config_update_event);

#endif

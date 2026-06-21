#ifndef BLE_SENSOR_LOGGER_STATUS_EVENTS_H_
#define BLE_SENSOR_LOGGER_STATUS_EVENTS_H_

#include <app_event_manager.h>
#include "protocol.h"

struct status_update_event {
	struct app_event_header header;
	struct bsl_status status;
};

APP_EVENT_TYPE_DECLARE(status_update_event);

#endif

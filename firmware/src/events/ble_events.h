#ifndef BLE_SENSOR_LOGGER_BLE_EVENTS_H_
#define BLE_SENSOR_LOGGER_BLE_EVENTS_H_

#include <app_event_manager.h>
#include "protocol.h"

struct ble_connected_event {
	struct app_event_header header;
};

struct ble_disconnected_event {
	struct app_event_header header;
	uint8_t reason;
};

struct pc_command_event {
	struct app_event_header header;
	struct bsl_control command;
};

APP_EVENT_TYPE_DECLARE(ble_connected_event);
APP_EVENT_TYPE_DECLARE(ble_disconnected_event);
APP_EVENT_TYPE_DECLARE(pc_command_event);

#endif

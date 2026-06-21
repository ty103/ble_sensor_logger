#include "ble_events.h"

static void log_ble_connected_event(const struct app_event_header *aeh)
{
	APP_EVENT_MANAGER_LOG(aeh, "connected");
}

static void log_ble_disconnected_event(const struct app_event_header *aeh)
{
	const struct ble_disconnected_event *event = cast_ble_disconnected_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "reason=%u", event->reason);
}

static void log_pc_command_event(const struct app_event_header *aeh)
{
	const struct pc_command_event *event = cast_pc_command_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "command=%u value=%u", event->command.command,
			      event->command.value);
}

APP_EVENT_TYPE_DEFINE(ble_connected_event,
		      log_ble_connected_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(ble_disconnected_event,
		      log_ble_disconnected_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(pc_command_event,
		      log_pc_command_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE());

#include "config_events.h"

static void log_config_update_event(const struct app_event_header *aeh)
{
	const struct config_update_event *event = cast_config_update_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "op=%u stream=%u sample_interval=%u",
			      event->config.op,
			      event->config.stream_id,
			      event->config.sample_interval_ms);
}

APP_EVENT_TYPE_DEFINE(config_update_event,
		      log_config_update_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE());

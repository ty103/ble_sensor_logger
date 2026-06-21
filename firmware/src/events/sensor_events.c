#include "sensor_events.h"

static void log_sensor_sample_event(const struct app_event_header *aeh)
{
	const struct sensor_sample_event *event = cast_sensor_sample_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "stream=%u seq=%u timestamp=%u", event->sample.header.stream_id,
			      event->sample.header.sequence, event->sample.header.timestamp_ms);
}

APP_EVENT_TYPE_DEFINE(sensor_sample_event,
		      log_sensor_sample_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE());

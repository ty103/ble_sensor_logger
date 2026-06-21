#include "status_events.h"

static void log_status_update_event(const struct app_event_header *aeh)
{
	const struct status_update_event *event = cast_status_update_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "state=%u error=%u",
			      event->status.state,
			      event->status.last_error);
}

APP_EVENT_TYPE_DEFINE(status_update_event,
		      log_status_update_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE());

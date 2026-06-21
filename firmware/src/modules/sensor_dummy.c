#include "sensor_dummy.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "platform_time.h"
#include "protocol.h"
#include "sensor_events.h"

LOG_MODULE_REGISTER(sensor_dummy, LOG_LEVEL_INF);

static struct k_work_delayable sample_work;
static uint16_t interval_ms = BSL_INTERVAL_DEFAULT_MS;
static uint16_t sequence;
static bool running;

static int16_t wave_value(uint16_t seq, int16_t base)
{
	return base + (int16_t)((seq % 40) * 10) - 200;
}

static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!running) {
		return;
	}

	struct sensor_sample_event *event = new_sensor_sample_event();

	event->sample.header.version = BSL_PROTOCOL_VERSION;
	event->sample.header.message_type = BSL_MESSAGE_TYPE_SENSOR_SAMPLE;
	event->sample.header.stream_id = BSL_STREAM_ID_DUMMY_ACCEL3;
	event->sample.header.flags = 0;
	event->sample.header.sequence = sequence++;
	event->sample.header.timestamp_ms = platform_uptime_ms();
	event->sample.header.payload_format = BSL_PAYLOAD_FORMAT_DUMMY_ACCEL3_INT16_V1;
	event->sample.header.payload_len = BSL_SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE;
	event->sample.payload.dummy_accel3.accel_x_mg = wave_value(sequence, 0);
	event->sample.payload.dummy_accel3.accel_y_mg = wave_value(sequence + 7, 0);
	event->sample.payload.dummy_accel3.accel_z_mg = wave_value(sequence + 13, 1000);
	APP_EVENT_SUBMIT(event);

	k_work_schedule(&sample_work, K_MSEC(interval_ms));
}

int sensor_dummy_init(void)
{
	k_work_init_delayable(&sample_work, sample_work_handler);
	return 0;
}

void sensor_dummy_start(void)
{
	if (running) {
		return;
	}
	running = true;
	k_work_schedule(&sample_work, K_NO_WAIT);
}

void sensor_dummy_stop(void)
{
	running = false;
	k_work_cancel_delayable(&sample_work);
}

void sensor_dummy_reset_sequence(void)
{
	sequence = 0;
}

void sensor_dummy_set_interval(uint16_t new_interval_ms)
{
	interval_ms = new_interval_ms;
	if (running) {
		k_work_reschedule(&sample_work, K_MSEC(interval_ms));
	}
}

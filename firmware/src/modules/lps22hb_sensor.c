#include "lps22hb_sensor.h"

#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "platform_time.h"
#include "protocol.h"
#include "sensor_events.h"

LOG_MODULE_REGISTER(lps22hb_sensor, LOG_LEVEL_INF);

static struct k_work_delayable sample_work;
static uint16_t sequence;
static uint16_t status_error = BSL_ERROR_LPS22HB_NO_DEVICETREE;
static bool running;
static bool ready;

#if DT_HAS_COMPAT_STATUS_OKAY(st_lps22hb_press)
static const struct device *const lps22hb = DEVICE_DT_GET_ONE(st_lps22hb_press);

static int32_t clamp_i32(int64_t value)
{
	if (value > INT32_MAX) {
		return INT32_MAX;
	}
	if (value < INT32_MIN) {
		return INT32_MIN;
	}
	return (int32_t)value;
}

static int32_t pressure_to_pa(const struct sensor_value *value)
{
	return clamp_i32(sensor_value_to_micro(value) / 1000LL);
}

static void fill_sample(struct bsl_sensor_data *sample,
			const struct sensor_value *pressure)
{
	sample->header.version = BSL_PROTOCOL_VERSION;
	sample->header.message_type = BSL_MESSAGE_TYPE_SENSOR_SAMPLE;
	sample->header.stream_id = BSL_STREAM_ID_LPS22HB_PRESSURE;
	sample->header.flags = 0;
	sample->header.sequence = sequence++;
	sample->header.timestamp_ms = platform_uptime_ms();
	sample->header.payload_format = BSL_PAYLOAD_FORMAT_LPS22HB_PRESSURE_INT32_V1;
	sample->header.payload_len = BSL_SENSOR_LPS22HB_SAMPLE_SIZE;
	sample->header.sample_count = 1;
	sample->payload.lps22hb.pressure_pa = pressure_to_pa(pressure);
}

static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!running || !ready) {
		return;
	}

	struct sensor_value pressure;
	int err = sensor_sample_fetch(lps22hb);

	if (err) {
		LOG_WRN("sample fetch failed: %d", err);
		goto reschedule;
	}

	err = sensor_channel_get(lps22hb, SENSOR_CHAN_PRESS, &pressure);
	if (err) {
		LOG_WRN("pressure read failed: %d", err);
		goto reschedule;
	}

	struct sensor_sample_event *event = new_sensor_sample_event();
	fill_sample(&event->sample, &pressure);
	APP_EVENT_SUBMIT(event);

reschedule:
	k_work_schedule(&sample_work, K_MSEC(BSL_LPS22HB_INTERVAL_MS));
}

int lps22hb_sensor_init(void)
{
	k_work_init_delayable(&sample_work, sample_work_handler);

	if (!device_is_ready(lps22hb)) {
		status_error = BSL_ERROR_LPS22HB_NOT_READY;
		LOG_ERR("%s is not ready", lps22hb->name);
		return -ENODEV;
	}

	status_error = BSL_ERROR_NONE;
	ready = true;
	LOG_INF("%s ready at %u ms interval", lps22hb->name, BSL_LPS22HB_INTERVAL_MS);
	return 0;
}
#else
static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

int lps22hb_sensor_init(void)
{
	k_work_init_delayable(&sample_work, sample_work_handler);
	status_error = BSL_ERROR_LPS22HB_NO_DEVICETREE;
	LOG_INF("LPS22HB stream disabled; no st,lps22hb-press devicetree node");
	return 0;
}
#endif

void lps22hb_sensor_start(void)
{
	if (running || !ready) {
		return;
	}
	running = true;
	k_work_schedule(&sample_work, K_NO_WAIT);
}

void lps22hb_sensor_stop(void)
{
	running = false;
	k_work_cancel_delayable(&sample_work);
}

void lps22hb_sensor_reset_sequence(void)
{
	sequence = 0;
}

bool lps22hb_sensor_is_available(void)
{
	return ready;
}

uint16_t lps22hb_sensor_status_error(void)
{
	return status_error;
}

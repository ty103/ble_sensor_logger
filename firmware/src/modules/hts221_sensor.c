#include "hts221_sensor.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "platform_time.h"
#include "protocol.h"
#include "sensor_events.h"

LOG_MODULE_REGISTER(hts221_sensor, LOG_LEVEL_INF);

static struct k_work_delayable sample_work;
static uint16_t sequence;
static uint16_t status_error = BSL_ERROR_HTS221_NO_DEVICETREE;
static bool running;
static bool ready;

#if DT_HAS_COMPAT_STATUS_OKAY(st_hts221)
static const struct device *const hts221 = DEVICE_DT_GET_ONE(st_hts221);

static int16_t clamp_i16(int64_t value)
{
	if (value > INT16_MAX) {
		return INT16_MAX;
	}
	if (value < INT16_MIN) {
		return INT16_MIN;
	}
	return (int16_t)value;
}

static int16_t value_to_centi(const struct sensor_value *value)
{
	return clamp_i16(sensor_value_to_micro(value) / 10000LL);
}

static void fill_sample(struct bsl_sensor_data *sample,
			const struct sensor_value *humidity,
			const struct sensor_value *temperature)
{
	sample->header.version = BSL_PROTOCOL_VERSION;
	sample->header.message_type = BSL_MESSAGE_TYPE_SENSOR_SAMPLE;
	sample->header.stream_id = BSL_STREAM_ID_HTS221_TEMP_HUMIDITY;
	sample->header.flags = 0;
	sample->header.sequence = sequence++;
	sample->header.timestamp_ms = platform_uptime_ms();
	sample->header.payload_format = BSL_PAYLOAD_FORMAT_HTS221_TEMP_HUMIDITY_INT16_V1;
	sample->header.payload_len = BSL_SENSOR_HTS221_SAMPLE_SIZE;
	sample->payload.hts221.humidity_centi_percent = value_to_centi(humidity);
	sample->payload.hts221.temperature_centi_c = value_to_centi(temperature);
}

static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!running || !ready) {
		return;
	}

	struct sensor_value humidity;
	struct sensor_value temperature;
	int err = sensor_sample_fetch(hts221);

	if (err) {
		LOG_WRN("sample fetch failed: %d", err);
		goto reschedule;
	}

	err = sensor_channel_get(hts221, SENSOR_CHAN_HUMIDITY, &humidity);
	if (err) {
		LOG_WRN("humidity read failed: %d", err);
		goto reschedule;
	}

	err = sensor_channel_get(hts221, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
	if (err) {
		LOG_WRN("temperature read failed: %d", err);
		goto reschedule;
	}

	struct sensor_sample_event *event = new_sensor_sample_event();
	fill_sample(&event->sample, &humidity, &temperature);
	APP_EVENT_SUBMIT(event);

reschedule:
	k_work_schedule(&sample_work, K_MSEC(BSL_HTS221_INTERVAL_MS));
}

int hts221_sensor_init(void)
{
	k_work_init_delayable(&sample_work, sample_work_handler);

	if (!device_is_ready(hts221)) {
		status_error = BSL_ERROR_HTS221_NOT_READY;
		LOG_ERR("%s is not ready", hts221->name);
		return -ENODEV;
	}

	status_error = BSL_ERROR_NONE;
	ready = true;
	LOG_INF("%s ready at %u ms interval", hts221->name, BSL_HTS221_INTERVAL_MS);
	return 0;
}
#else
static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

int hts221_sensor_init(void)
{
	k_work_init_delayable(&sample_work, sample_work_handler);
	status_error = BSL_ERROR_HTS221_NO_DEVICETREE;
	LOG_INF("HTS221 stream disabled; no st,hts221 devicetree node");
	return 0;
}
#endif

void hts221_sensor_start(void)
{
	if (running || !ready) {
		return;
	}
	running = true;
	k_work_schedule(&sample_work, K_NO_WAIT);
}

void hts221_sensor_stop(void)
{
	running = false;
	k_work_cancel_delayable(&sample_work);
}

void hts221_sensor_reset_sequence(void)
{
	sequence = 0;
}

bool hts221_sensor_is_available(void)
{
	return ready;
}

uint16_t hts221_sensor_status_error(void)
{
	return status_error;
}

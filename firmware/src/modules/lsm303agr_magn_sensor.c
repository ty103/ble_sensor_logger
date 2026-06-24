#include "lsm303agr_magn_sensor.h"

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

LOG_MODULE_REGISTER(lsm303agr_magn_sensor, LOG_LEVEL_INF);

static struct k_work_delayable sample_work;
static uint16_t sequence;
static uint16_t status_error = BSL_ERROR_LSM303AGR_MAGN_NO_DEVICETREE;
static bool running;
static bool ready;

#if DT_NODE_HAS_STATUS(DT_ALIAS(magn0), okay)
static const struct device *const lsm303agr_magn = DEVICE_DT_GET(DT_ALIAS(magn0));

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

static int16_t magn_to_uT(const struct sensor_value *value)
{
	int64_t micro_gauss = sensor_value_to_micro(value);

	return clamp_i16(micro_gauss / 10000LL);
}

static void fill_sample(struct bsl_sensor_data *sample,
			const struct sensor_value mag[3])
{
	sample->header.version = BSL_PROTOCOL_VERSION;
	sample->header.message_type = BSL_MESSAGE_TYPE_SENSOR_SAMPLE;
	sample->header.stream_id = BSL_STREAM_ID_LSM303AGR_MAG3;
	sample->header.flags = 0;
	sample->header.sequence = sequence++;
	sample->header.timestamp_ms = platform_uptime_ms();
	sample->header.payload_format = BSL_PAYLOAD_FORMAT_MAG3_INT16_V1;
	sample->header.payload_len = BSL_SENSOR_MAG3_SAMPLE_SIZE;
	sample->header.sample_count = 1;
	sample->payload.mag3.mag_x_uT = magn_to_uT(&mag[0]);
	sample->payload.mag3.mag_y_uT = magn_to_uT(&mag[1]);
	sample->payload.mag3.mag_z_uT = magn_to_uT(&mag[2]);
}

static int configure_sensor(void)
{
	struct sensor_value odr = {
		.val1 = 10,
		.val2 = 0,
	};

	return sensor_attr_set(lsm303agr_magn, SENSOR_CHAN_MAGN_XYZ,
			       SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
}

static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!running || !ready) {
		return;
	}

	struct sensor_value mag[3];
	int err = sensor_sample_fetch(lsm303agr_magn);

	if (err) {
		LOG_WRN("sample fetch failed: %d", err);
		goto reschedule;
	}

	err = sensor_channel_get(lsm303agr_magn, SENSOR_CHAN_MAGN_XYZ, mag);
	if (err) {
		LOG_WRN("magnetometer read failed: %d", err);
		goto reschedule;
	}

	struct sensor_sample_event *event = new_sensor_sample_event();
	fill_sample(&event->sample, mag);
	APP_EVENT_SUBMIT(event);

reschedule:
	k_work_schedule(&sample_work, K_MSEC(BSL_LSM303AGR_MAG3_INTERVAL_MS));
}

int lsm303agr_magn_sensor_init(void)
{
	k_work_init_delayable(&sample_work, sample_work_handler);

	if (!device_is_ready(lsm303agr_magn)) {
		status_error = BSL_ERROR_LSM303AGR_MAGN_NOT_READY;
		LOG_ERR("%s is not ready", lsm303agr_magn->name);
		return -ENODEV;
	}

	int err = configure_sensor();

	if (err) {
		status_error = BSL_ERROR_LSM303AGR_MAGN_NOT_READY;
		LOG_ERR("failed to configure %s: %d", lsm303agr_magn->name, err);
		return err;
	}

	status_error = BSL_ERROR_NONE;
	ready = true;
	LOG_INF("%s ready at %u ms interval", lsm303agr_magn->name,
		BSL_LSM303AGR_MAG3_INTERVAL_MS);
	return 0;
}
#else
static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

int lsm303agr_magn_sensor_init(void)
{
	k_work_init_delayable(&sample_work, sample_work_handler);
	status_error = BSL_ERROR_LSM303AGR_MAGN_NO_DEVICETREE;
	LOG_INF("LSM303AGR magnetometer stream disabled; no magn0 devicetree alias");
	return 0;
}
#endif

void lsm303agr_magn_sensor_start(void)
{
	if (running || !ready) {
		return;
	}
	running = true;
	k_work_schedule(&sample_work, K_NO_WAIT);
}

void lsm303agr_magn_sensor_stop(void)
{
	running = false;
	k_work_cancel_delayable(&sample_work);
}

void lsm303agr_magn_sensor_reset_sequence(void)
{
	sequence = 0;
}

bool lsm303agr_magn_sensor_is_available(void)
{
	return ready;
}

uint16_t lsm303agr_magn_sensor_status_error(void)
{
	return status_error;
}

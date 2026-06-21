#include "lsm6dsl_sensor.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "platform_time.h"
#include "protocol.h"
#include "sensor_events.h"

LOG_MODULE_REGISTER(lsm6dsl_sensor, LOG_LEVEL_INF);

static struct k_work_delayable sample_work;
static uint16_t sequence;
static uint16_t status_error = BSL_ERROR_LSM6DSL_NO_DEVICETREE;
static bool running;
static bool ready;

#if DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dsl)
static const struct device *const lsm6dsl = DEVICE_DT_GET_ONE(st_lsm6dsl);

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

static int16_t accel_to_mg(const struct sensor_value *value)
{
	int64_t micro_mps2 = sensor_value_to_micro(value);

	return clamp_i16((micro_mps2 * 100LL) / 980665LL);
}

static int16_t gyro_to_mdps(const struct sensor_value *value)
{
	int64_t micro_rad_s = sensor_value_to_micro(value);

	return clamp_i16((micro_rad_s * 57296LL) / 1000000LL);
}

static void fill_sample(struct bsl_sensor_data *sample,
			const struct sensor_value accel[3],
			const struct sensor_value gyro[3])
{
	sample->header.version = BSL_PROTOCOL_VERSION;
	sample->header.message_type = BSL_MESSAGE_TYPE_SENSOR_SAMPLE;
	sample->header.stream_id = BSL_STREAM_ID_LSM6DSL_IMU6;
	sample->header.flags = 0;
	sample->header.sequence = sequence++;
	sample->header.timestamp_ms = platform_uptime_ms();
	sample->header.payload_format = BSL_PAYLOAD_FORMAT_IMU6_INT16_V1;
	sample->header.payload_len = BSL_SENSOR_IMU6_SAMPLE_SIZE;
	sample->payload.imu6.accel_x_mg = accel_to_mg(&accel[0]);
	sample->payload.imu6.accel_y_mg = accel_to_mg(&accel[1]);
	sample->payload.imu6.accel_z_mg = accel_to_mg(&accel[2]);
	sample->payload.imu6.gyro_x_mdps = gyro_to_mdps(&gyro[0]);
	sample->payload.imu6.gyro_y_mdps = gyro_to_mdps(&gyro[1]);
	sample->payload.imu6.gyro_z_mdps = gyro_to_mdps(&gyro[2]);
}

static int configure_sensor(void)
{
	struct sensor_value odr = {
		.val1 = 26,
		.val2 = 0,
	};
	int err;

	err = sensor_attr_set(lsm6dsl, SENSOR_CHAN_ACCEL_XYZ,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
	if (err) {
		LOG_ERR("failed to set accel ODR: %d", err);
		return err;
	}

	err = sensor_attr_set(lsm6dsl, SENSOR_CHAN_GYRO_XYZ,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
	if (err) {
		LOG_ERR("failed to set gyro ODR: %d", err);
		return err;
	}

	return 0;
}

static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!running || !ready) {
		return;
	}

	struct sensor_value accel[3];
	struct sensor_value gyro[3];
	int err = sensor_sample_fetch(lsm6dsl);

	if (err) {
		LOG_WRN("sample fetch failed: %d", err);
		goto reschedule;
	}

	err = sensor_channel_get(lsm6dsl, SENSOR_CHAN_ACCEL_XYZ, accel);
	if (err) {
		LOG_WRN("accel read failed: %d", err);
		goto reschedule;
	}

	err = sensor_channel_get(lsm6dsl, SENSOR_CHAN_GYRO_XYZ, gyro);
	if (err) {
		LOG_WRN("gyro read failed: %d", err);
		goto reschedule;
	}

	struct sensor_sample_event *event = new_sensor_sample_event();
	fill_sample(&event->sample, accel, gyro);
	APP_EVENT_SUBMIT(event);

reschedule:
	k_work_schedule(&sample_work, K_MSEC(BSL_LSM6DSL_INTERVAL_MS));
}

int lsm6dsl_sensor_init(void)
{
	k_work_init_delayable(&sample_work, sample_work_handler);

	if (!device_is_ready(lsm6dsl)) {
		status_error = BSL_ERROR_LSM6DSL_NOT_READY;
		LOG_ERR("%s is not ready", lsm6dsl->name);
		return -ENODEV;
	}

	int err = configure_sensor();

	if (err) {
		status_error = BSL_ERROR_LSM6DSL_CONFIG_FAILED;
		return err;
	}

	status_error = BSL_ERROR_NONE;
	ready = true;
	LOG_INF("%s ready at %u ms interval", lsm6dsl->name, BSL_LSM6DSL_INTERVAL_MS);
	return 0;
}
#else
static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

int lsm6dsl_sensor_init(void)
{
	k_work_init_delayable(&sample_work, sample_work_handler);
	status_error = BSL_ERROR_LSM6DSL_NO_DEVICETREE;
	LOG_INF("LSM6DSL stream disabled; no st,lsm6dsl devicetree node");
	return 0;
}
#endif

void lsm6dsl_sensor_start(void)
{
	if (running || !ready) {
		return;
	}
	running = true;
	k_work_schedule(&sample_work, K_NO_WAIT);
}

void lsm6dsl_sensor_stop(void)
{
	running = false;
	k_work_cancel_delayable(&sample_work);
}

void lsm6dsl_sensor_reset_sequence(void)
{
	sequence = 0;
}

bool lsm6dsl_sensor_is_available(void)
{
	return ready;
}

uint16_t lsm6dsl_sensor_status_error(void)
{
	return status_error;
}

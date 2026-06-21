#include "iks01a2_i2c_probe.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(iks01a2_i2c_probe, LOG_LEVEL_INF);

#define IKS01A2_PROBE_INTERVAL_MS 1000
#define IKS01A2_DEFAULT_WHO_AM_I_REG 0x0f
#define LIS2MDL_WHO_AM_I_REG 0x4f

struct i2c_probe_target {
	const char *name;
	uint8_t address;
	uint8_t who_am_i_reg;
	uint8_t expected_who_am_i;
};

static const struct device *const arduino_i2c = DEVICE_DT_GET(DT_NODELABEL(arduino_i2c));
static struct k_work_delayable periodic_probe_work;

static const struct i2c_probe_target i2c_probe_targets[] = {
	{ "HTS221", 0x5f, IKS01A2_DEFAULT_WHO_AM_I_REG, 0xbc },
	{ "LPS22HB", 0x5d, IKS01A2_DEFAULT_WHO_AM_I_REG, 0xb1 },
	{ "LSM6DSL", 0x6b, IKS01A2_DEFAULT_WHO_AM_I_REG, 0x6a },
	{ "LSM303AGR accel", 0x19, IKS01A2_DEFAULT_WHO_AM_I_REG, 0x33 },
	{ "LSM303AGR magn", 0x1e, LIS2MDL_WHO_AM_I_REG, 0x40 },
};

static void probe_lsm303agr_sensor_api(void);

static void probe_raw_who_am_i(const struct i2c_probe_target *target)
{
	uint8_t reg = target->who_am_i_reg;
	uint8_t value = 0;
	int err = i2c_write(arduino_i2c, &reg, sizeof(reg), target->address);

	if (err) {
		LOG_WRN("%s@0x%02x WHO_AM_I register 0x%02x write returned %d; forcing read",
			target->name, target->address, target->who_am_i_reg, err);
	} else {
		LOG_INF("%s@0x%02x WHO_AM_I register 0x%02x write ok",
			target->name, target->address, target->who_am_i_reg);
	}

	err = i2c_read(arduino_i2c, &value, sizeof(value), target->address);
	if (err) {
		LOG_WRN("%s@0x%02x WHO_AM_I data read failed after register ACK: %d",
			target->name, target->address, err);
		return;
	}

	LOG_INF("%s@0x%02x WHO_AM_I[0x%02x]=0x%02x expected=0x%02x",
		target->name, target->address, target->who_am_i_reg, value,
		target->expected_who_am_i);
}

static void periodic_probe_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!device_is_ready(arduino_i2c)) {
		LOG_ERR("arduino_i2c bus is not ready");
		goto reschedule;
	}

	for (size_t i = 0; i < ARRAY_SIZE(i2c_probe_targets); i++) {
		probe_raw_who_am_i(&i2c_probe_targets[i]);
	}

	probe_lsm303agr_sensor_api();

reschedule:
	k_work_schedule(&periodic_probe_work, K_MSEC(IKS01A2_PROBE_INTERVAL_MS));
}

static void log_sensor_value(const char *name, const char *channel,
			     const struct sensor_value *value)
{
	LOG_INF("%s %s=%d.%06d", name, channel, value->val1, value->val2);
}

static void probe_sensor_channel(const struct device *dev,
				 enum sensor_channel channel,
				 const char *channel_name,
				 const char *device_name)
{
	struct sensor_value value;
	int err = sensor_channel_get(dev, channel, &value);

	if (err) {
		LOG_WRN("%s %s read failed: %d", device_name, channel_name, err);
		return;
	}

	log_sensor_value(device_name, channel_name, &value);
}

static void probe_sensor_channel_xyz(const struct device *dev,
				     enum sensor_channel channel,
				     const char *channel_name,
				     const char *device_name)
{
	struct sensor_value values[3];
	int err = sensor_channel_get(dev, channel, values);

	if (err) {
		LOG_WRN("%s %s read failed: %d", device_name, channel_name, err);
		return;
	}

	LOG_INF("%s %s=(%d.%06d, %d.%06d, %d.%06d)",
		device_name, channel_name,
		values[0].val1, values[0].val2,
		values[1].val1, values[1].val2,
		values[2].val1, values[2].val2);
}

static void probe_sensor(const struct device *dev, const char *device_name,
			 enum sensor_channel channel_a, const char *channel_a_name,
			 enum sensor_channel channel_b, const char *channel_b_name)
{
	int err;

	if (!device_is_ready(dev)) {
		LOG_ERR("%s is not ready", device_name);
		return;
	}

	LOG_INF("%s is ready", device_name);

	err = sensor_sample_fetch(dev);
	if (err) {
		LOG_WRN("%s sample fetch failed: %d", device_name, err);
		return;
	}

	probe_sensor_channel(dev, channel_a, channel_a_name, device_name);
	probe_sensor_channel(dev, channel_b, channel_b_name, device_name);
}

static void probe_sensor_xyz(const struct device *dev, const char *device_name,
			     enum sensor_channel channel,
			     const char *channel_name)
{
	int err;

	if (!device_is_ready(dev)) {
		LOG_ERR("%s is not ready", device_name);
		return;
	}

	LOG_INF("%s is ready", device_name);

	err = sensor_sample_fetch(dev);
	if (err) {
		LOG_WRN("%s sample fetch failed: %d", device_name, err);
		return;
	}

	probe_sensor_channel_xyz(dev, channel, channel_name, device_name);
}

static void probe_lsm303agr_sensor_api(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(magn0))
	const struct device *const lsm303agr_magn = DEVICE_DT_GET(DT_ALIAS(magn0));

	probe_sensor_xyz(lsm303agr_magn, "LSM303AGR magn@0x1e",
			 SENSOR_CHAN_MAGN_XYZ, "mag_xyz");
#else
	LOG_INF("LSM303AGR magnetometer probe skipped; no magn0 devicetree alias");
#endif

#if DT_NODE_EXISTS(DT_ALIAS(accel0))
	const struct device *const lsm303agr_accel = DEVICE_DT_GET(DT_ALIAS(accel0));

	probe_sensor_xyz(lsm303agr_accel, "LSM303AGR accel@0x19",
			 SENSOR_CHAN_ACCEL_XYZ, "accel_xyz");
#else
	LOG_INF("LSM303AGR accelerometer probe skipped; no accel0 devicetree alias");
#endif
}

void iks01a2_i2c_probe_run(void)
{
	k_work_init_delayable(&periodic_probe_work, periodic_probe_handler);
	k_work_schedule(&periodic_probe_work, K_NO_WAIT);

#if DT_HAS_COMPAT_STATUS_OKAY(st_hts221)
	const struct device *const hts221 = DEVICE_DT_GET_ONE(st_hts221);

	probe_sensor(hts221, "HTS221@0x5f",
		     SENSOR_CHAN_HUMIDITY, "humidity",
		     SENSOR_CHAN_AMBIENT_TEMP, "temperature");
#else
	LOG_INF("HTS221 probe skipped; no st,hts221 devicetree node");
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_lps22hb_press)
	const struct device *const lps22hb = DEVICE_DT_GET_ONE(st_lps22hb_press);

	probe_sensor(lps22hb, "LPS22HB@0x5d",
		     SENSOR_CHAN_PRESS, "pressure",
		     SENSOR_CHAN_AMBIENT_TEMP, "temperature");
#else
	LOG_INF("LPS22HB probe skipped; no st,lps22hb-press devicetree node");
#endif

	probe_lsm303agr_sensor_api();
}

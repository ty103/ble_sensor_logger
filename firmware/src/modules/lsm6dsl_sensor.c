#include "lsm6dsl_sensor.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "platform_time.h"
#include "protocol.h"
#include "sensor_events.h"

LOG_MODULE_REGISTER(lsm6dsl_sensor, LOG_LEVEL_INF);

#define BSL_ORIENTATION_COMPLEMENTARY_ALPHA_DEFAULT 0.98
#define BSL_ORIENTATION_MAHONY_KP_DEFAULT 0.5
#define BSL_ORIENTATION_MAHONY_KI_DEFAULT 0.0
#define BSL_ORIENTATION_IIR_CUTOFF_HZ_DEFAULT 2.0
#define BSL_PI 3.14159265358979323846

static struct k_work_delayable sample_work;
static uint16_t sequence;
static uint16_t status_error = BSL_ERROR_LSM6DSL_NO_DEVICETREE;
static bool running;
static bool ready;
static bool orientation_filter_initialized;
static uint32_t orientation_last_timestamp_ms;
static double complementary_alpha = BSL_ORIENTATION_COMPLEMENTARY_ALPHA_DEFAULT;
static double mahony_kp = BSL_ORIENTATION_MAHONY_KP_DEFAULT;
static double mahony_ki = BSL_ORIENTATION_MAHONY_KI_DEFAULT;
static double iir_cutoff_hz = BSL_ORIENTATION_IIR_CUTOFF_HZ_DEFAULT;
static double iir_pitch_deg;
static double iir_roll_deg;
static double iir_zenith_deg;
static double complementary_pitch_deg;
static double complementary_roll_deg;
static double mahony_integral[3];
static double mahony_q[4] = {1.0, 0.0, 0.0, 0.0};

/* Gyro auto-calibration state */
static double gyro_bias[3];
static uint32_t gyro_still_counter;
static bool gyro_force_calib_requested;
static uint16_t calib_threshold_mdps = CONFIG_BSL_GYRO_CALIB_THRESHOLD_MDPS;
static uint16_t calib_alpha_permille = CONFIG_BSL_GYRO_CALIB_ALPHA_PERMILLE;
static uint16_t calib_window_samples = CONFIG_BSL_GYRO_CALIB_WINDOW_SAMPLES;

#if DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dsl)
static const struct device *const lsm6dsl = DEVICE_DT_GET_ONE(st_lsm6dsl);

struct signed_axis {
	uint8_t index;
	int8_t sign;
};

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

static int16_t double_to_i16(double value)
{
	if (value > (double)INT16_MAX) {
		return INT16_MAX;
	}
	if (value < (double)INT16_MIN) {
		return INT16_MIN;
	}
	return (int16_t)llround(value);
}

static double clamp_double(double value, double min_value, double max_value)
{
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
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

static bool parse_axis(const char *name, struct signed_axis *axis)
{
	if (!name || !axis || strlen(name) != 2U) {
		return false;
	}
	if (name[0] == '+') {
		axis->sign = 1;
	} else if (name[0] == '-') {
		axis->sign = -1;
	} else {
		return false;
	}

	switch (name[1]) {
	case 'X':
		axis->index = 0;
		return true;
	case 'Y':
		axis->index = 1;
		return true;
	case 'Z':
		axis->index = 2;
		return true;
	default:
		return false;
	}
}

static bool load_orientation_axes(struct signed_axis *front_axis,
				  struct signed_axis *gravity_axis)
{
	if (!parse_axis(CONFIG_BSL_ORIENTATION_FRONT_AXIS, front_axis) ||
	    !parse_axis(CONFIG_BSL_ORIENTATION_GRAVITY_AXIS, gravity_axis)) {
		return false;
	}
	return front_axis->index != gravity_axis->index;
}

static void axis_to_vector(const struct signed_axis *axis, int8_t vector[3])
{
	vector[0] = 0;
	vector[1] = 0;
	vector[2] = 0;
	vector[axis->index] = axis->sign;
}

static void cross_i8(const int8_t a[3], const int8_t b[3], int8_t out[3])
{
	out[0] = (int8_t)(a[1] * b[2] - a[2] * b[1]);
	out[1] = (int8_t)(a[2] * b[0] - a[0] * b[2]);
	out[2] = (int8_t)(a[0] * b[1] - a[1] * b[0]);
}

static double project_axis(const double values[3], const int8_t axis[3])
{
	return values[0] * axis[0] + values[1] * axis[1] + values[2] * axis[2];
}

static void project_to_device_axes(const double raw[3], double device[3])
{
	struct signed_axis front_axis;
	struct signed_axis gravity_axis;
	int8_t front[3];
	int8_t gravity[3];
	int8_t lateral[3];

	if (!load_orientation_axes(&front_axis, &gravity_axis)) {
		device[0] = raw[0];
		device[1] = raw[1];
		device[2] = raw[2];
		return;
	}

	axis_to_vector(&front_axis, front);
	axis_to_vector(&gravity_axis, gravity);
	cross_i8(gravity, front, lateral);

	device[0] = project_axis(raw, front);
	device[1] = project_axis(raw, lateral);
	device[2] = project_axis(raw, gravity);
}

static int16_t deg_to_cdeg(double deg)
{
	return double_to_i16(deg * 100.0);
}

static double rad_to_deg(double rad)
{
	return rad * (180.0 / BSL_PI);
}

static double zenith_from_pitch_roll(double pitch_deg, double roll_deg)
{
	double pitch_rad = pitch_deg * (BSL_PI / 180.0);
	double roll_rad = roll_deg * (BSL_PI / 180.0);
	double z_component = cos(pitch_rad) * cos(roll_rad);

	return rad_to_deg(acos(clamp_double(z_component, -1.0, 1.0)));
}

static double iir_alpha_from_cutoff(double cutoff_hz, double dt_s)
{
	double rc_s = 1.0 / (2.0 * BSL_PI * cutoff_hz);

	return clamp_double(dt_s / (rc_s + dt_s), 0.0, 1.0);
}

static void normalize_quaternion(void)
{
	double norm = sqrt((mahony_q[0] * mahony_q[0]) + (mahony_q[1] * mahony_q[1]) +
			   (mahony_q[2] * mahony_q[2]) + (mahony_q[3] * mahony_q[3]));

	if (norm <= 0.0) {
		mahony_q[0] = 1.0;
		mahony_q[1] = 0.0;
		mahony_q[2] = 0.0;
		mahony_q[3] = 0.0;
		return;
	}
	for (size_t i = 0; i < ARRAY_SIZE(mahony_q); i++) {
		mahony_q[i] /= norm;
	}
}

static void initialize_mahony_quaternion(double pitch_deg, double roll_deg)
{
	double pitch = pitch_deg * (BSL_PI / 180.0);
	double roll = roll_deg * (BSL_PI / 180.0);
	double cr = cos(roll * 0.5);
	double sr = sin(roll * 0.5);
	double cp = cos(pitch * 0.5);
	double sp = sin(pitch * 0.5);

	mahony_q[0] = cr * cp;
	mahony_q[1] = sr * cp;
	mahony_q[2] = cr * sp;
	mahony_q[3] = -sr * sp;
	mahony_integral[0] = 0.0;
	mahony_integral[1] = 0.0;
	mahony_integral[2] = 0.0;
	normalize_quaternion();
}

static void update_mahony(const double accel_mg[3], const double gyro_mdps[3], double dt_s)
{
	double accel_norm = sqrt((accel_mg[0] * accel_mg[0]) + (accel_mg[1] * accel_mg[1]) +
				 (accel_mg[2] * accel_mg[2]));
	double gx = gyro_mdps[0] * (BSL_PI / 180.0) / 1000.0;
	double gy = gyro_mdps[1] * (BSL_PI / 180.0) / 1000.0;
	double gz = gyro_mdps[2] * (BSL_PI / 180.0) / 1000.0;
	double q0 = mahony_q[0];
	double q1 = mahony_q[1];
	double q2 = mahony_q[2];
	double q3 = mahony_q[3];
	double q_dot[4];

	if (accel_norm > 0.0) {
		double ax = accel_mg[0] / accel_norm;
		double ay = accel_mg[1] / accel_norm;
		double az = accel_mg[2] / accel_norm;
		double vx = 2.0 * ((q1 * q3) - (q0 * q2));
		double vy = 2.0 * ((q0 * q1) + (q2 * q3));
		double vz = (q0 * q0) - (q1 * q1) - (q2 * q2) + (q3 * q3);
		double error[3] = {
			(ay * vz) - (az * vy),
			(az * vx) - (ax * vz),
			(ax * vy) - (ay * vx),
		};

		if (mahony_ki > 0.0) {
			for (size_t i = 0; i < ARRAY_SIZE(mahony_integral); i++) {
				mahony_integral[i] += mahony_ki * error[i] * dt_s;
			}
		} else {
			mahony_integral[0] = 0.0;
			mahony_integral[1] = 0.0;
			mahony_integral[2] = 0.0;
		}

		gx += (mahony_kp * error[0]) + mahony_integral[0];
		gy += (mahony_kp * error[1]) + mahony_integral[1];
		gz += (mahony_kp * error[2]) + mahony_integral[2];
	}

	q_dot[0] = -0.5 * ((q1 * gx) + (q2 * gy) + (q3 * gz));
	q_dot[1] = 0.5 * ((q0 * gx) + (q2 * gz) - (q3 * gy));
	q_dot[2] = 0.5 * ((q0 * gy) - (q1 * gz) + (q3 * gx));
	q_dot[3] = 0.5 * ((q0 * gz) + (q1 * gy) - (q2 * gx));

	mahony_q[0] += q_dot[0] * dt_s;
	mahony_q[1] += q_dot[1] * dt_s;
	mahony_q[2] += q_dot[2] * dt_s;
	mahony_q[3] += q_dot[3] * dt_s;
	normalize_quaternion();
}

static void mahony_to_euler(double *pitch_deg, double *roll_deg, double *yaw_deg)
{
	double q0 = mahony_q[0];
	double q1 = mahony_q[1];
	double q2 = mahony_q[2];
	double q3 = mahony_q[3];
	double sin_pitch = 2.0 * ((q0 * q2) - (q3 * q1));

	*roll_deg = rad_to_deg(atan2(2.0 * ((q0 * q1) + (q2 * q3)),
				     1.0 - (2.0 * ((q1 * q1) + (q2 * q2)))));
	*pitch_deg = rad_to_deg(asin(clamp_double(sin_pitch, -1.0, 1.0)));
	*yaw_deg = rad_to_deg(atan2(2.0 * ((q0 * q3) + (q1 * q2)),
				    1.0 - (2.0 * ((q2 * q2) + (q3 * q3)))));
}

static void reset_orientation_filters(void)
{
	orientation_filter_initialized = false;
	orientation_last_timestamp_ms = 0;
	complementary_pitch_deg = 0.0;
	complementary_roll_deg = 0.0;
	iir_pitch_deg = 0.0;
	iir_roll_deg = 0.0;
	iir_zenith_deg = 0.0;
	mahony_integral[0] = 0.0;
	mahony_integral[1] = 0.0;
	mahony_integral[2] = 0.0;
	mahony_q[0] = 1.0;
	mahony_q[1] = 0.0;
	mahony_q[2] = 0.0;
	mahony_q[3] = 0.0;
	gyro_bias[0] = 0.0;
	gyro_bias[1] = 0.0;
	gyro_bias[2] = 0.0;
	gyro_still_counter = 0;
	gyro_force_calib_requested = false;
}

static void fill_header(struct bsl_sensor_data *sample, uint8_t stream_id,
			uint8_t payload_format, uint8_t payload_len,
			uint16_t sample_sequence, uint32_t timestamp_ms)
{
	sample->header.version = BSL_PROTOCOL_VERSION;
	sample->header.message_type = BSL_MESSAGE_TYPE_SENSOR_SAMPLE;
	sample->header.stream_id = stream_id;
	sample->header.flags = 0;
	sample->header.sequence = sample_sequence;
	sample->header.timestamp_ms = timestamp_ms;
	sample->header.payload_format = payload_format;
	sample->header.payload_len = payload_len;
	sample->header.sample_count = 1;
}

static void fill_imu6_sample(struct bsl_sensor_data *sample,
			     const int16_t accel_mg[3],
			     const int16_t gyro_mdps[3],
			     uint16_t sample_sequence,
			     uint32_t timestamp_ms)
{
	fill_header(sample, BSL_STREAM_ID_LSM6DSL_IMU6, BSL_PAYLOAD_FORMAT_IMU6_INT16_V1,
		    BSL_SENSOR_IMU6_SAMPLE_SIZE, sample_sequence, timestamp_ms);
	sample->payload.imu6.accel_x_mg = accel_mg[0];
	sample->payload.imu6.accel_y_mg = accel_mg[1];
	sample->payload.imu6.accel_z_mg = accel_mg[2];
	sample->payload.imu6.gyro_x_mdps = gyro_mdps[0];
	sample->payload.imu6.gyro_y_mdps = gyro_mdps[1];
	sample->payload.imu6.gyro_z_mdps = gyro_mdps[2];
}

static void fill_orientation_sample(struct bsl_sensor_data *sample,
				    const int16_t accel_mg[3],
				    const int16_t gyro_mdps[3],
				    uint16_t sample_sequence,
				    uint32_t timestamp_ms)
{
	double raw_accel[3] = {
		(double)accel_mg[0],
		(double)accel_mg[1],
		(double)accel_mg[2],
	};
	double raw_gyro[3] = {
		(double)gyro_mdps[0],
		(double)gyro_mdps[1],
		(double)gyro_mdps[2],
	};
	double accel[3];
	double gyro[3];
	double horizontal_norm;
	double accel_norm;
	double pitch_naive_deg;
	double roll_naive_deg;
	double zenith_naive_deg;
	double dt_s = (double)BSL_LSM6DSL_INTERVAL_MS / 1000.0;
	double iir_alpha;
	double gyro_pitch_delta_deg;
	double gyro_roll_delta_deg;
	double mahony_pitch_deg;
	double mahony_roll_deg;
	double mahony_zenith_deg;
	double mahony_yaw_deg;

	project_to_device_axes(raw_accel, accel);
	project_to_device_axes(raw_gyro, gyro);

	horizontal_norm = sqrt((accel[0] * accel[0]) + (accel[1] * accel[1]));
	accel_norm = sqrt((accel[0] * accel[0]) + (accel[1] * accel[1]) +
			  (accel[2] * accel[2]));
	pitch_naive_deg = rad_to_deg(atan2(-accel[0], sqrt((accel[1] * accel[1]) +
							   (accel[2] * accel[2]))));
	roll_naive_deg = rad_to_deg(atan2(accel[1], accel[2]));
	zenith_naive_deg = rad_to_deg(atan2(horizontal_norm, accel[2]));

	if (!orientation_filter_initialized) {
		complementary_pitch_deg = pitch_naive_deg;
		complementary_roll_deg = roll_naive_deg;
		iir_pitch_deg = pitch_naive_deg;
		iir_roll_deg = roll_naive_deg;
		iir_zenith_deg = zenith_naive_deg;
		initialize_mahony_quaternion(pitch_naive_deg, roll_naive_deg);
		orientation_filter_initialized = true;
	} else {
		if (timestamp_ms >= orientation_last_timestamp_ms) {
			dt_s = (double)(timestamp_ms - orientation_last_timestamp_ms) / 1000.0;
		} else {
			dt_s = (double)BSL_LSM6DSL_INTERVAL_MS / 1000.0;
		}
		dt_s = clamp_double(dt_s, 0.001, 1.0);
		iir_alpha = iir_alpha_from_cutoff(iir_cutoff_hz, dt_s);
		iir_pitch_deg += iir_alpha * (pitch_naive_deg - iir_pitch_deg);
		iir_roll_deg += iir_alpha * (roll_naive_deg - iir_roll_deg);
		iir_zenith_deg += iir_alpha * (zenith_naive_deg - iir_zenith_deg);
		gyro_pitch_delta_deg = (gyro[1] / 1000.0) * dt_s;
		gyro_roll_delta_deg = (gyro[0] / 1000.0) * dt_s;
		complementary_pitch_deg =
			(complementary_alpha * (complementary_pitch_deg + gyro_pitch_delta_deg)) +
			((1.0 - complementary_alpha) * pitch_naive_deg);
		complementary_roll_deg =
			(complementary_alpha * (complementary_roll_deg + gyro_roll_delta_deg)) +
			((1.0 - complementary_alpha) * roll_naive_deg);
		update_mahony(accel, gyro, dt_s);
	}
	orientation_last_timestamp_ms = timestamp_ms;
	mahony_to_euler(&mahony_pitch_deg, &mahony_roll_deg, &mahony_yaw_deg);
	mahony_zenith_deg = zenith_from_pitch_roll(mahony_pitch_deg, mahony_roll_deg);

	fill_header(sample, BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION,
		    BSL_PAYLOAD_FORMAT_ORIENTATION_MOTION_INT16_V2,
		    BSL_SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE, sample_sequence, timestamp_ms);
	sample->payload.orientation_motion.pitch_naive_cdeg = deg_to_cdeg(pitch_naive_deg);
	sample->payload.orientation_motion.roll_naive_cdeg = deg_to_cdeg(roll_naive_deg);
	sample->payload.orientation_motion.zenith_naive_cdeg = deg_to_cdeg(zenith_naive_deg);
	sample->payload.orientation_motion.pitch_iir_cdeg = deg_to_cdeg(iir_pitch_deg);
	sample->payload.orientation_motion.roll_iir_cdeg = deg_to_cdeg(iir_roll_deg);
	sample->payload.orientation_motion.zenith_iir_cdeg = deg_to_cdeg(iir_zenith_deg);
	sample->payload.orientation_motion.pitch_complementary_cdeg =
		deg_to_cdeg(complementary_pitch_deg);
	sample->payload.orientation_motion.roll_complementary_cdeg =
		deg_to_cdeg(complementary_roll_deg);
	sample->payload.orientation_motion.zenith_complementary_cdeg =
		deg_to_cdeg(zenith_from_pitch_roll(complementary_pitch_deg,
						   complementary_roll_deg));
	sample->payload.orientation_motion.pitch_mahony_cdeg = deg_to_cdeg(mahony_pitch_deg);
	sample->payload.orientation_motion.roll_mahony_cdeg = deg_to_cdeg(mahony_roll_deg);
	sample->payload.orientation_motion.zenith_mahony_cdeg = deg_to_cdeg(mahony_zenith_deg);
	sample->payload.orientation_motion.yaw_mahony_cdeg = deg_to_cdeg(mahony_yaw_deg);
	sample->payload.orientation_motion.accel_norm_mg = double_to_i16(accel_norm);
}

/* Apply current bias to gyro_mdps[] in-place. */
static void apply_gyro_calib(int16_t gyro_mdps[3])
{
	for (size_t i = 0; i < 3; i++) {
		gyro_mdps[i] = clamp_i16((int64_t)gyro_mdps[i] - (int64_t)llround(gyro_bias[i]));
	}
}

/*
 * Update bias estimate.
 * raw_mdps:        raw (uncalibrated) gyro reading
 * calibrated_mdps: output after applying existing bias (for stillness check)
 */
static void update_gyro_calib(const int16_t raw_mdps[3], const int16_t calibrated_mdps[3])
{
	bool is_still = (abs(calibrated_mdps[0]) <= (int16_t)calib_threshold_mdps) &&
			(abs(calibrated_mdps[1]) <= (int16_t)calib_threshold_mdps) &&
			(abs(calibrated_mdps[2]) <= (int16_t)calib_threshold_mdps);

	if (is_still) {
		if (gyro_still_counter < UINT32_MAX) {
			gyro_still_counter++;
		}
	} else {
		gyro_still_counter = 0;
	}

	if (gyro_force_calib_requested) {
		for (size_t i = 0; i < 3; i++) {
			gyro_bias[i] = (double)raw_mdps[i];
		}
		gyro_still_counter = 0;
		gyro_force_calib_requested = false;
		LOG_INF("gyro force calib: bias=[%.1f %.1f %.1f] mdps",
			gyro_bias[0], gyro_bias[1], gyro_bias[2]);
	} else if (gyro_still_counter >= (uint32_t)calib_window_samples) {
		double alpha = (double)calib_alpha_permille / 1000.0;

		for (size_t i = 0; i < 3; i++) {
			gyro_bias[i] += alpha * ((double)raw_mdps[i] - gyro_bias[i]);
		}
	}
}

static void convert_sensor_values(const struct sensor_value accel[3],
				  const struct sensor_value gyro[3],
				  int16_t accel_mg[3],
				  int16_t gyro_mdps[3])
{
	accel_mg[0] = accel_to_mg(&accel[0]);
	accel_mg[1] = accel_to_mg(&accel[1]);
	accel_mg[2] = accel_to_mg(&accel[2]);
	gyro_mdps[0] = gyro_to_mdps(&gyro[0]);
	gyro_mdps[1] = gyro_to_mdps(&gyro[1]);
	gyro_mdps[2] = gyro_to_mdps(&gyro[2]);
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
	int16_t accel_mg[3];
	int16_t gyro_mdps[3];
	uint16_t sample_sequence;
	uint32_t timestamp_ms;
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

	convert_sensor_values(accel, gyro, accel_mg, gyro_mdps);

	int16_t gyro_raw[3] = {gyro_mdps[0], gyro_mdps[1], gyro_mdps[2]};

	apply_gyro_calib(gyro_mdps);
	update_gyro_calib(gyro_raw, gyro_mdps);

	sample_sequence = sequence++;
	timestamp_ms = platform_uptime_ms();

	struct sensor_sample_event *imu6_event = new_sensor_sample_event();
	fill_imu6_sample(&imu6_event->sample, accel_mg, gyro_mdps, sample_sequence,
			 timestamp_ms);
	APP_EVENT_SUBMIT(imu6_event);

	struct sensor_sample_event *orientation_event = new_sensor_sample_event();
	fill_orientation_sample(&orientation_event->sample, accel_mg, gyro_mdps,
				sample_sequence, timestamp_ms);
	APP_EVENT_SUBMIT(orientation_event);

reschedule:
	k_work_schedule(&sample_work, K_MSEC(BSL_LSM6DSL_INTERVAL_MS));
}

int lsm6dsl_sensor_init(void)
{
	struct signed_axis front_axis;
	struct signed_axis gravity_axis;

	k_work_init_delayable(&sample_work, sample_work_handler);

	if (!load_orientation_axes(&front_axis, &gravity_axis)) {
		status_error = BSL_ERROR_LSM6DSL_CONFIG_FAILED;
		LOG_ERR("invalid orientation axes: front=%s gravity=%s",
			CONFIG_BSL_ORIENTATION_FRONT_AXIS,
			CONFIG_BSL_ORIENTATION_GRAVITY_AXIS);
		return -EINVAL;
	}

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
	reset_orientation_filters();
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
	reset_orientation_filters();
}

bool lsm6dsl_sensor_is_available(void)
{
	return ready;
}

uint16_t lsm6dsl_sensor_status_error(void)
{
	return status_error;
}

void lsm6dsl_sensor_set_complementary_alpha_permille(uint16_t alpha_permille)
{
	complementary_alpha = clamp_double((double)alpha_permille / 1000.0, 0.0, 1.0);
}

void lsm6dsl_sensor_set_mahony_kp_milli(uint16_t kp_milli)
{
	mahony_kp = clamp_double((double)kp_milli / 1000.0, 0.0, 10.0);
}

void lsm6dsl_sensor_set_mahony_ki_milli(uint16_t ki_milli)
{
	mahony_ki = clamp_double((double)ki_milli / 1000.0, 0.0, 10.0);
}

void lsm6dsl_sensor_set_iir_cutoff_millihz(uint16_t cutoff_millihz)
{
	iir_cutoff_hz = clamp_double((double)cutoff_millihz / 1000.0, 0.001, 13.0);
}

void lsm6dsl_sensor_force_gyro_calib(void)
{
	gyro_force_calib_requested = true;
}

void lsm6dsl_sensor_set_gyro_calib_threshold_mdps(uint16_t threshold_mdps)
{
	if (threshold_mdps < 10U) {
		threshold_mdps = 10U;
	} else if (threshold_mdps > 1000U) {
		threshold_mdps = 1000U;
	}
	calib_threshold_mdps = threshold_mdps;
}

void lsm6dsl_sensor_set_gyro_calib_alpha_permille(uint16_t alpha_permille)
{
	if (alpha_permille > 1000U) {
		alpha_permille = 1000U;
	}
	calib_alpha_permille = alpha_permille;
}

void lsm6dsl_sensor_set_gyro_calib_window_samples(uint16_t window_samples)
{
	if (window_samples < 5U) {
		window_samples = 5U;
	} else if (window_samples > 260U) {
		window_samples = 260U;
	}
	calib_window_samples = window_samples;
}

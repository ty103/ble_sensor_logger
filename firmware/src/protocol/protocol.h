#ifndef BLE_SENSOR_LOGGER_PROTOCOL_H_
#define BLE_SENSOR_LOGGER_PROTOCOL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define BSL_PACKED __attribute__((__packed__))
#else
#define BSL_PACKED
#endif

#define BSL_PROTOCOL_VERSION 4
#define BSL_CONFIG_VERSION 4

#define BSL_SENSOR_DATA_HEADER_SIZE 13
#define BSL_SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE 6
#define BSL_SENSOR_IMU6_SAMPLE_SIZE 12
#define BSL_SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE 28
#define BSL_SENSOR_MAG3_SAMPLE_SIZE 6
#define BSL_SENSOR_HTS221_SAMPLE_SIZE 4
#define BSL_SENSOR_LPS22HB_SAMPLE_SIZE 4
#define BSL_SENSOR_BATCH_FLUSH_MS 100
#ifdef CONFIG_BSL_LATENCY_PRIORITY
#define BSL_SENSOR_BATCH_MAX_SAMPLES_IMU6 1
#define BSL_SENSOR_BATCH_MAX_SAMPLES_ORIENTATION 1
#else
#define BSL_SENSOR_BATCH_MAX_SAMPLES_IMU6 8
#define BSL_SENSOR_BATCH_MAX_SAMPLES_ORIENTATION 4
#endif
#define BSL_SENSOR_DATA_MAX_PAYLOAD_SIZE \
	(BSL_SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE * BSL_SENSOR_BATCH_MAX_SAMPLES_ORIENTATION)
#define BSL_SENSOR_DATA_SIZE (BSL_SENSOR_DATA_HEADER_SIZE + BSL_SENSOR_DATA_MAX_PAYLOAD_SIZE)
#define BSL_CONTROL_SIZE 4
#define BSL_CONFIG_SIZE 8
#define BSL_STATUS_SIZE 16
#define BSL_CAPABILITY_HEADER_SIZE 12
#define BSL_CAPABILITY_STREAM_SIZE 16
#define BSL_CAPABILITY_MAX_STREAMS 6
#define BSL_CAPABILITY_SIZE \
	(BSL_CAPABILITY_HEADER_SIZE + (BSL_CAPABILITY_STREAM_SIZE * BSL_CAPABILITY_MAX_STREAMS))

#define BSL_CAPABILITY_SCHEMA_VERSION 1

#define BSL_INTERVAL_MIN_MS 20
#define BSL_INTERVAL_MAX_MS 10000
#define BSL_INTERVAL_DEFAULT_MS 100

#define BSL_STREAM_ID_DUMMY_ACCEL3 1
#define BSL_STREAM_ID_LSM6DSL_IMU6 10
#define BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION 13
#define BSL_STREAM_ID_LSM303AGR_MAG3 12
#define BSL_STREAM_ID_LPS22HB_PRESSURE 20
#define BSL_STREAM_ID_HTS221_TEMP_HUMIDITY 30
#define BSL_LSM6DSL_INTERVAL_MS 38
#define BSL_LSM303AGR_MAG3_INTERVAL_MS 100
#define BSL_LPS22HB_INTERVAL_MS 1000
#define BSL_HTS221_INTERVAL_MS 1000

enum bsl_message_type {
	BSL_MESSAGE_TYPE_SENSOR_SAMPLE = 0x01,
};

enum bsl_command {
	BSL_COMMAND_START_MEASUREMENT = 0x01,
	BSL_COMMAND_STOP_MEASUREMENT = 0x02,
	BSL_COMMAND_REQUEST_STATUS = 0x03,
	BSL_COMMAND_RESET_SEQUENCE = 0x04,
	BSL_COMMAND_FORCE_GYRO_CALIB = 0x05,
};

enum bsl_config_op {
	BSL_CONFIG_OP_SET_STREAM_INTERVAL = 0x01,
	BSL_CONFIG_OP_SET_COMPLEMENTARY_ALPHA = 0x02,
	BSL_CONFIG_OP_SET_MAHONY_KP = 0x03,
	BSL_CONFIG_OP_SET_MAHONY_KI = 0x04,
	BSL_CONFIG_OP_SET_IIR_CUTOFF_MILLIHZ = 0x05,
	/* Gyro auto-calibration parameters (stream_id=10, value in sample_interval_ms field) */
	BSL_CONFIG_OP_SET_GYRO_CALIB_THRESHOLD = 0x06,  /* still threshold in mdps */
	BSL_CONFIG_OP_SET_GYRO_CALIB_ALPHA = 0x07,      /* bias lerp rate in permille (0-1000) */
	BSL_CONFIG_OP_SET_GYRO_CALIB_WINDOW = 0x08,     /* consecutive still samples required */
};

enum bsl_device_state {
	BSL_DEVICE_STATE_IDLE = 0,
	BSL_DEVICE_STATE_MEASURING = 1,
	BSL_DEVICE_STATE_ERROR = 2,
};

enum bsl_device_error {
	BSL_ERROR_NONE = 0,
	BSL_ERROR_INVALID_VERSION = 1,
	BSL_ERROR_INVALID_LENGTH = 2,
	BSL_ERROR_INVALID_COMMAND = 3,
	BSL_ERROR_INVALID_CONFIG = 4,
	BSL_ERROR_NOT_CONNECTED = 5,
	BSL_ERROR_NOT_SUBSCRIBED = 6,
	BSL_ERROR_LSM6DSL_NO_DEVICETREE = 7,
	BSL_ERROR_LSM6DSL_NOT_READY = 8,
	BSL_ERROR_LSM6DSL_CONFIG_FAILED = 9,
	BSL_ERROR_HTS221_NO_DEVICETREE = 10,
	BSL_ERROR_HTS221_NOT_READY = 11,
	BSL_ERROR_LPS22HB_NO_DEVICETREE = 12,
	BSL_ERROR_LPS22HB_NOT_READY = 13,
	BSL_ERROR_LSM303AGR_MAGN_NO_DEVICETREE = 14,
	BSL_ERROR_LSM303AGR_MAGN_NOT_READY = 15,
};

enum bsl_capability_flag {
	BSL_CAPABILITY_FLAG_CUSTOM_GATT = 0x0001,
	BSL_CAPABILITY_FLAG_FIXED_BINARY = 0x0002,
};

enum bsl_capability_feature {
	BSL_CAPABILITY_FEATURE_INTERVAL_CONFIG = 0x0001,
	BSL_CAPABILITY_FEATURE_STATUS_READ = 0x0002,
	BSL_CAPABILITY_FEATURE_SENSOR_NOTIFY = 0x0004,
};

enum bsl_stream_type {
	BSL_STREAM_TYPE_DUMMY_ACCEL3 = 1,
	BSL_STREAM_TYPE_IMU6 = 2,
	BSL_STREAM_TYPE_TEMP_HUMIDITY = 3,
	BSL_STREAM_TYPE_PRESSURE = 4,
	BSL_STREAM_TYPE_MAG3 = 5,
	BSL_STREAM_TYPE_ORIENTATION_MOTION = 6,
};

enum bsl_stream_data_type {
	BSL_STREAM_DATA_TYPE_INT16 = 1,
	BSL_STREAM_DATA_TYPE_INT32 = 2,
};

enum bsl_stream_unit {
	BSL_STREAM_UNIT_MIXED = 0,
	BSL_STREAM_UNIT_PA = 1,
	BSL_STREAM_UNIT_UT = 2,
	BSL_STREAM_UNIT_MG = 3,
};

enum bsl_payload_format {
	BSL_PAYLOAD_FORMAT_DUMMY_ACCEL3_INT16_V1 = 1,
	BSL_PAYLOAD_FORMAT_IMU6_INT16_V1 = 2,
	BSL_PAYLOAD_FORMAT_HTS221_TEMP_HUMIDITY_INT16_V1 = 3,
	BSL_PAYLOAD_FORMAT_LPS22HB_PRESSURE_INT32_V1 = 4,
	BSL_PAYLOAD_FORMAT_MAG3_INT16_V1 = 5,
	BSL_PAYLOAD_FORMAT_ORIENTATION_MOTION_INT16_V1 = 6,
	BSL_PAYLOAD_FORMAT_ORIENTATION_MOTION_INT16_V2 = 7,
};

enum bsl_stream_flag {
	BSL_STREAM_FLAG_ENABLED_BY_DEFAULT = 0x0001,
	BSL_STREAM_FLAG_MIXED_UNITS = 0x0002,
};

struct bsl_sensor_header {
	uint8_t version;
	uint8_t message_type;
	uint8_t stream_id;
	uint8_t flags;
	uint16_t sequence;
	uint32_t timestamp_ms;
	uint8_t payload_format;
	uint8_t payload_len;
	uint8_t sample_count;
} BSL_PACKED;

struct bsl_dummy_accel3_sample {
	int16_t accel_x_mg;
	int16_t accel_y_mg;
	int16_t accel_z_mg;
} BSL_PACKED;

struct bsl_imu6_sample {
	int16_t accel_x_mg;
	int16_t accel_y_mg;
	int16_t accel_z_mg;
	int16_t gyro_x_mdps;
	int16_t gyro_y_mdps;
	int16_t gyro_z_mdps;
} BSL_PACKED;

struct bsl_orientation_motion_sample {
	int16_t pitch_naive_cdeg;
	int16_t roll_naive_cdeg;
	int16_t zenith_naive_cdeg;
	int16_t pitch_iir_cdeg;
	int16_t roll_iir_cdeg;
	int16_t zenith_iir_cdeg;
	int16_t pitch_complementary_cdeg;
	int16_t roll_complementary_cdeg;
	int16_t zenith_complementary_cdeg;
	int16_t pitch_mahony_cdeg;
	int16_t roll_mahony_cdeg;
	int16_t zenith_mahony_cdeg;
	int16_t yaw_mahony_cdeg;
	int16_t accel_norm_mg;
} BSL_PACKED;

struct bsl_mag3_sample {
	int16_t mag_x_uT;
	int16_t mag_y_uT;
	int16_t mag_z_uT;
} BSL_PACKED;

struct bsl_hts221_sample {
	int16_t humidity_centi_percent;
	int16_t temperature_centi_c;
} BSL_PACKED;

struct bsl_lps22hb_sample {
	int32_t pressure_pa;
} BSL_PACKED;

struct bsl_sensor_data {
	struct bsl_sensor_header header;
	union {
		struct bsl_dummy_accel3_sample dummy_accel3;
		struct bsl_imu6_sample imu6;
		struct bsl_orientation_motion_sample orientation_motion;
		struct bsl_mag3_sample mag3;
		struct bsl_hts221_sample hts221;
		struct bsl_lps22hb_sample lps22hb;
		uint8_t raw[BSL_SENSOR_DATA_MAX_PAYLOAD_SIZE];
	} payload;
} BSL_PACKED;

struct bsl_control {
	uint8_t version;
	uint8_t command;
	uint16_t value;
} BSL_PACKED;

struct bsl_config {
	uint8_t version;
	uint8_t op;
	uint8_t stream_id;
	uint8_t flags;
	uint16_t sample_interval_ms;
	uint16_t reserved;
} BSL_PACKED;

struct bsl_status {
	uint8_t version;
	uint8_t state;
	uint16_t sample_interval_ms;
	uint16_t last_error;
	uint16_t connection_count;
	uint16_t lsm6dsl_error;
	uint16_t hts221_error;
	uint16_t lps22hb_error;
	uint16_t lsm303agr_magn_error;
} BSL_PACKED;

struct bsl_capability_header {
	uint8_t version;
	uint8_t schema_version;
	uint16_t capability_flags;
	uint8_t stream_count;
	uint8_t reserved;
	uint16_t supported_commands;
	uint16_t supported_features;
	uint16_t preferred_mtu;
} BSL_PACKED;

struct bsl_capability_stream {
	uint8_t stream_id;
	uint8_t stream_type;
	uint8_t channel_count;
	uint8_t data_type;
	uint8_t unit;
	uint8_t payload_format;
	uint16_t stream_flags;
	uint16_t default_interval_ms;
	uint16_t min_interval_ms;
	uint16_t max_interval_ms;
	int8_t scale_exponent;
	uint8_t reserved;
} BSL_PACKED;

struct bsl_capability {
	struct bsl_capability_header header;
	struct bsl_capability_stream streams[BSL_CAPABILITY_MAX_STREAMS];
} BSL_PACKED;

int bsl_sensor_data_pack(const struct bsl_sensor_data *payload, uint8_t *buf, size_t len);
size_t bsl_sensor_data_size(const struct bsl_sensor_data *payload);
int bsl_control_unpack(const uint8_t *buf, size_t len, struct bsl_control *payload);
int bsl_config_pack(const struct bsl_config *payload, uint8_t *buf, size_t len);
int bsl_config_unpack(const uint8_t *buf, size_t len, struct bsl_config *payload);
int bsl_status_pack(const struct bsl_status *payload, uint8_t *buf, size_t len);
int bsl_capability_pack(const struct bsl_capability *payload, uint8_t *buf, size_t len);
size_t bsl_capability_size(const struct bsl_capability *payload);

bool bsl_config_is_valid(const struct bsl_config *payload);
void bsl_config_default(struct bsl_config *payload);
void bsl_capability_default(struct bsl_capability *payload);

#endif

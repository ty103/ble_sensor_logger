#include "protocol.h"

#include <errno.h>
#include <string.h>

_Static_assert(sizeof(struct bsl_sensor_header) == BSL_SENSOR_DATA_HEADER_SIZE,
	       "unexpected sensor data header size");
_Static_assert(sizeof(struct bsl_dummy_accel3_sample) == BSL_SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE,
	       "unexpected dummy accel3 sample payload size");
_Static_assert(sizeof(struct bsl_imu6_sample) == BSL_SENSOR_IMU6_SAMPLE_SIZE,
	       "unexpected IMU6 sample payload size");
_Static_assert(sizeof(struct bsl_orientation_motion_sample) ==
		       BSL_SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE,
	       "unexpected orientation motion sample payload size");
_Static_assert(sizeof(struct bsl_mag3_sample) == BSL_SENSOR_MAG3_SAMPLE_SIZE,
	       "unexpected MAG3 sample payload size");
_Static_assert(sizeof(struct bsl_hts221_sample) == BSL_SENSOR_HTS221_SAMPLE_SIZE,
	       "unexpected HTS221 sample payload size");
_Static_assert(sizeof(struct bsl_lps22hb_sample) == BSL_SENSOR_LPS22HB_SAMPLE_SIZE,
	       "unexpected LPS22HB sample payload size");
_Static_assert(sizeof(struct bsl_sensor_data) == BSL_SENSOR_DATA_SIZE,
	       "unexpected sensor data max payload size");
_Static_assert(sizeof(struct bsl_control) == BSL_CONTROL_SIZE,
	       "unexpected control payload size");
_Static_assert(sizeof(struct bsl_config) == BSL_CONFIG_SIZE,
	       "unexpected config payload size");
_Static_assert(sizeof(struct bsl_status) == BSL_STATUS_SIZE,
	       "unexpected status payload size");
_Static_assert(sizeof(struct bsl_capability_header) == BSL_CAPABILITY_HEADER_SIZE,
	       "unexpected capability header size");
_Static_assert(sizeof(struct bsl_capability_stream) == BSL_CAPABILITY_STREAM_SIZE,
	       "unexpected capability stream size");
_Static_assert(sizeof(struct bsl_capability) == BSL_CAPABILITY_SIZE,
	       "unexpected capability payload size");

int bsl_sensor_data_pack(const struct bsl_sensor_data *payload, uint8_t *buf, size_t len)
{
	size_t frame_size;

	if (!payload || !buf) {
		return -EINVAL;
	}
	frame_size = bsl_sensor_data_size(payload);
	if (frame_size == 0U || len < frame_size) {
		return -EINVAL;
	}
	if (payload->header.version != BSL_PROTOCOL_VERSION) {
		return -EPROTO;
	}
	if (payload->header.message_type != BSL_MESSAGE_TYPE_SENSOR_SAMPLE) {
		return -EPROTO;
	}
	if (payload->header.stream_id == BSL_STREAM_ID_DUMMY_ACCEL3) {
		if (payload->header.payload_format != BSL_PAYLOAD_FORMAT_DUMMY_ACCEL3_INT16_V1 ||
		    payload->header.payload_len != BSL_SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE) {
			return -EPROTO;
		}
	} else if (payload->header.stream_id == BSL_STREAM_ID_LSM6DSL_IMU6) {
		if (payload->header.payload_format != BSL_PAYLOAD_FORMAT_IMU6_INT16_V1 ||
		    payload->header.payload_len != BSL_SENSOR_IMU6_SAMPLE_SIZE) {
			return -EPROTO;
		}
	} else if (payload->header.stream_id == BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION) {
		if (payload->header.payload_format !=
			    BSL_PAYLOAD_FORMAT_ORIENTATION_MOTION_INT16_V2 ||
		    payload->header.payload_len != BSL_SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE) {
			return -EPROTO;
		}
	} else if (payload->header.stream_id == BSL_STREAM_ID_LSM303AGR_MAG3) {
		if (payload->header.payload_format != BSL_PAYLOAD_FORMAT_MAG3_INT16_V1 ||
		    payload->header.payload_len != BSL_SENSOR_MAG3_SAMPLE_SIZE) {
			return -EPROTO;
		}
	} else if (payload->header.stream_id == BSL_STREAM_ID_HTS221_TEMP_HUMIDITY) {
		if (payload->header.payload_format !=
			    BSL_PAYLOAD_FORMAT_HTS221_TEMP_HUMIDITY_INT16_V1 ||
		    payload->header.payload_len != BSL_SENSOR_HTS221_SAMPLE_SIZE) {
			return -EPROTO;
		}
	} else if (payload->header.stream_id == BSL_STREAM_ID_LPS22HB_PRESSURE) {
		if (payload->header.payload_format != BSL_PAYLOAD_FORMAT_LPS22HB_PRESSURE_INT32_V1 ||
		    payload->header.payload_len != BSL_SENSOR_LPS22HB_SAMPLE_SIZE) {
			return -EPROTO;
		}
	} else {
		return -EPROTO;
	}
	memcpy(buf, payload, frame_size);
	return (int)frame_size;
}

size_t bsl_sensor_data_size(const struct bsl_sensor_data *payload)
{
	if (!payload ||
	    payload->header.payload_len == 0U ||
	    payload->header.payload_len > BSL_SENSOR_DATA_MAX_PAYLOAD_SIZE) {
		return 0U;
	}
	return BSL_SENSOR_DATA_HEADER_SIZE + payload->header.payload_len;
}

int bsl_control_unpack(const uint8_t *buf, size_t len, struct bsl_control *payload)
{
	if (!buf || !payload || len != BSL_CONTROL_SIZE) {
		return -EINVAL;
	}

	memcpy(payload, buf, BSL_CONTROL_SIZE);
	if (payload->version != BSL_PROTOCOL_VERSION) {
		return -EPROTO;
	}

	switch (payload->command) {
	case BSL_COMMAND_START_MEASUREMENT:
	case BSL_COMMAND_STOP_MEASUREMENT:
	case BSL_COMMAND_REQUEST_STATUS:
	case BSL_COMMAND_RESET_SEQUENCE:
		return 0;
	default:
		return -ENOTSUP;
	}
}

int bsl_config_pack(const struct bsl_config *payload, uint8_t *buf, size_t len)
{
	if (!payload || !buf || len != BSL_CONFIG_SIZE) {
		return -EINVAL;
	}
	if (!bsl_config_is_valid(payload)) {
		return -EINVAL;
	}
	memcpy(buf, payload, BSL_CONFIG_SIZE);
	return 0;
}

int bsl_config_unpack(const uint8_t *buf, size_t len, struct bsl_config *payload)
{
	if (!buf || !payload || len != BSL_CONFIG_SIZE) {
		return -EINVAL;
	}
	memcpy(payload, buf, BSL_CONFIG_SIZE);
	return bsl_config_is_valid(payload) ? 0 : -EINVAL;
}

int bsl_status_pack(const struct bsl_status *payload, uint8_t *buf, size_t len)
{
	if (!payload || !buf || len != BSL_STATUS_SIZE) {
		return -EINVAL;
	}
	if (payload->version != BSL_PROTOCOL_VERSION) {
		return -EPROTO;
	}
	memcpy(buf, payload, BSL_STATUS_SIZE);
	return 0;
}

int bsl_capability_pack(const struct bsl_capability *payload, uint8_t *buf, size_t len)
{
	size_t payload_size;

	if (!payload || !buf) {
		return -EINVAL;
	}
	payload_size = bsl_capability_size(payload);
	if (payload_size == 0U || len < payload_size) {
		return -EINVAL;
	}
	if (payload->header.version != BSL_PROTOCOL_VERSION ||
	    payload->header.schema_version != BSL_CAPABILITY_SCHEMA_VERSION ||
	    payload->header.stream_count > BSL_CAPABILITY_MAX_STREAMS) {
		return -EPROTO;
	}
	memcpy(buf, payload, payload_size);
	return (int)payload_size;
}

size_t bsl_capability_size(const struct bsl_capability *payload)
{
	if (!payload || payload->header.stream_count > BSL_CAPABILITY_MAX_STREAMS) {
		return 0U;
	}
	return BSL_CAPABILITY_HEADER_SIZE +
	       (payload->header.stream_count * BSL_CAPABILITY_STREAM_SIZE);
}

bool bsl_config_is_valid(const struct bsl_config *payload)
{
	if (!payload) {
		return false;
	}
	if (payload->version != BSL_CONFIG_VERSION || payload->reserved != 0 ||
	    payload->flags != 0) {
		return false;
	}
	if (payload->op == BSL_CONFIG_OP_SET_STREAM_INTERVAL) {
		if (payload->stream_id != BSL_STREAM_ID_DUMMY_ACCEL3) {
			return false;
		}
		if (payload->sample_interval_ms < BSL_INTERVAL_MIN_MS ||
		    payload->sample_interval_ms > BSL_INTERVAL_MAX_MS) {
			return false;
		}
		return true;
	}
	if (payload->stream_id != BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION) {
		return false;
	}
	switch (payload->op) {
	case BSL_CONFIG_OP_SET_COMPLEMENTARY_ALPHA:
		return payload->sample_interval_ms <= 1000U;
	case BSL_CONFIG_OP_SET_MAHONY_KP:
	case BSL_CONFIG_OP_SET_MAHONY_KI:
		return payload->sample_interval_ms <= 10000U;
	case BSL_CONFIG_OP_SET_IIR_CUTOFF_MILLIHZ:
		return payload->sample_interval_ms >= 1U && payload->sample_interval_ms <= 13000U;
	default:
		return false;
	}
}

void bsl_config_default(struct bsl_config *payload)
{
	if (!payload) {
		return;
	}
	payload->version = BSL_CONFIG_VERSION;
	payload->op = BSL_CONFIG_OP_SET_STREAM_INTERVAL;
	payload->stream_id = BSL_STREAM_ID_DUMMY_ACCEL3;
	payload->flags = 0;
	payload->sample_interval_ms = BSL_INTERVAL_DEFAULT_MS;
	payload->reserved = 0;
}

void bsl_capability_default(struct bsl_capability *payload)
{
	if (!payload) {
		return;
	}

	memset(payload, 0, sizeof(*payload));
	payload->header.version = BSL_PROTOCOL_VERSION;
	payload->header.schema_version = BSL_CAPABILITY_SCHEMA_VERSION;
	payload->header.capability_flags = BSL_CAPABILITY_FLAG_CUSTOM_GATT |
					   BSL_CAPABILITY_FLAG_FIXED_BINARY;
	payload->header.stream_count = BSL_CAPABILITY_MAX_STREAMS;
	payload->header.supported_commands = (1U << BSL_COMMAND_START_MEASUREMENT) |
					     (1U << BSL_COMMAND_STOP_MEASUREMENT) |
					     (1U << BSL_COMMAND_REQUEST_STATUS) |
					     (1U << BSL_COMMAND_RESET_SEQUENCE);
	payload->header.supported_features = BSL_CAPABILITY_FEATURE_INTERVAL_CONFIG |
					     BSL_CAPABILITY_FEATURE_STATUS_READ |
					     BSL_CAPABILITY_FEATURE_SENSOR_NOTIFY;
	payload->header.preferred_mtu = BSL_SENSOR_DATA_SIZE;

	payload->streams[0].stream_id = BSL_STREAM_ID_DUMMY_ACCEL3;
	payload->streams[0].stream_type = BSL_STREAM_TYPE_DUMMY_ACCEL3;
	payload->streams[0].channel_count = 3;
	payload->streams[0].data_type = BSL_STREAM_DATA_TYPE_INT16;
	payload->streams[0].unit = BSL_STREAM_UNIT_MG;
	payload->streams[0].payload_format = BSL_PAYLOAD_FORMAT_DUMMY_ACCEL3_INT16_V1;
	payload->streams[0].stream_flags = BSL_STREAM_FLAG_ENABLED_BY_DEFAULT;
	payload->streams[0].default_interval_ms = BSL_INTERVAL_DEFAULT_MS;
	payload->streams[0].min_interval_ms = BSL_INTERVAL_MIN_MS;
	payload->streams[0].max_interval_ms = BSL_INTERVAL_MAX_MS;
	payload->streams[0].scale_exponent = 0;

	payload->streams[1].stream_id = BSL_STREAM_ID_LSM6DSL_IMU6;
	payload->streams[1].stream_type = BSL_STREAM_TYPE_IMU6;
	payload->streams[1].channel_count = 6;
	payload->streams[1].data_type = BSL_STREAM_DATA_TYPE_INT16;
	payload->streams[1].unit = BSL_STREAM_UNIT_MIXED;
	payload->streams[1].payload_format = BSL_PAYLOAD_FORMAT_IMU6_INT16_V1;
	payload->streams[1].stream_flags = BSL_STREAM_FLAG_ENABLED_BY_DEFAULT |
					   BSL_STREAM_FLAG_MIXED_UNITS;
	payload->streams[1].default_interval_ms = BSL_LSM6DSL_INTERVAL_MS;
	payload->streams[1].min_interval_ms = BSL_LSM6DSL_INTERVAL_MS;
	payload->streams[1].max_interval_ms = BSL_LSM6DSL_INTERVAL_MS;
	payload->streams[1].scale_exponent = 0;

	payload->streams[2].stream_id = BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION;
	payload->streams[2].stream_type = BSL_STREAM_TYPE_ORIENTATION_MOTION;
	payload->streams[2].channel_count = 14;
	payload->streams[2].data_type = BSL_STREAM_DATA_TYPE_INT16;
	payload->streams[2].unit = BSL_STREAM_UNIT_MIXED;
	payload->streams[2].payload_format = BSL_PAYLOAD_FORMAT_ORIENTATION_MOTION_INT16_V2;
	payload->streams[2].stream_flags = BSL_STREAM_FLAG_ENABLED_BY_DEFAULT |
					   BSL_STREAM_FLAG_MIXED_UNITS;
	payload->streams[2].default_interval_ms = BSL_LSM6DSL_INTERVAL_MS;
	payload->streams[2].min_interval_ms = BSL_LSM6DSL_INTERVAL_MS;
	payload->streams[2].max_interval_ms = BSL_LSM6DSL_INTERVAL_MS;
	payload->streams[2].scale_exponent = -2;

	payload->streams[3].stream_id = BSL_STREAM_ID_HTS221_TEMP_HUMIDITY;
	payload->streams[3].stream_type = BSL_STREAM_TYPE_TEMP_HUMIDITY;
	payload->streams[3].channel_count = 2;
	payload->streams[3].data_type = BSL_STREAM_DATA_TYPE_INT16;
	payload->streams[3].unit = BSL_STREAM_UNIT_MIXED;
	payload->streams[3].payload_format = BSL_PAYLOAD_FORMAT_HTS221_TEMP_HUMIDITY_INT16_V1;
	payload->streams[3].stream_flags = BSL_STREAM_FLAG_ENABLED_BY_DEFAULT |
					   BSL_STREAM_FLAG_MIXED_UNITS;
	payload->streams[3].default_interval_ms = BSL_HTS221_INTERVAL_MS;
	payload->streams[3].min_interval_ms = BSL_HTS221_INTERVAL_MS;
	payload->streams[3].max_interval_ms = BSL_HTS221_INTERVAL_MS;
	payload->streams[3].scale_exponent = -2;

	payload->streams[4].stream_id = BSL_STREAM_ID_LPS22HB_PRESSURE;
	payload->streams[4].stream_type = BSL_STREAM_TYPE_PRESSURE;
	payload->streams[4].channel_count = 1;
	payload->streams[4].data_type = BSL_STREAM_DATA_TYPE_INT32;
	payload->streams[4].unit = BSL_STREAM_UNIT_PA;
	payload->streams[4].payload_format = BSL_PAYLOAD_FORMAT_LPS22HB_PRESSURE_INT32_V1;
	payload->streams[4].stream_flags = BSL_STREAM_FLAG_ENABLED_BY_DEFAULT;
	payload->streams[4].default_interval_ms = BSL_LPS22HB_INTERVAL_MS;
	payload->streams[4].min_interval_ms = BSL_LPS22HB_INTERVAL_MS;
	payload->streams[4].max_interval_ms = BSL_LPS22HB_INTERVAL_MS;
	payload->streams[4].scale_exponent = 0;

	payload->streams[5].stream_id = BSL_STREAM_ID_LSM303AGR_MAG3;
	payload->streams[5].stream_type = BSL_STREAM_TYPE_MAG3;
	payload->streams[5].channel_count = 3;
	payload->streams[5].data_type = BSL_STREAM_DATA_TYPE_INT16;
	payload->streams[5].unit = BSL_STREAM_UNIT_UT;
	payload->streams[5].payload_format = BSL_PAYLOAD_FORMAT_MAG3_INT16_V1;
	payload->streams[5].stream_flags = BSL_STREAM_FLAG_ENABLED_BY_DEFAULT;
	payload->streams[5].default_interval_ms = BSL_LSM303AGR_MAG3_INTERVAL_MS;
	payload->streams[5].min_interval_ms = BSL_LSM303AGR_MAG3_INTERVAL_MS;
	payload->streams[5].max_interval_ms = BSL_LSM303AGR_MAG3_INTERVAL_MS;
	payload->streams[5].scale_exponent = 0;
}

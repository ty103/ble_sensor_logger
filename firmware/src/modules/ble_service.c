#include "ble_service.h"

#include <errno.h>
#include <string.h>
#include <app_event_manager.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ble_events.h"
#include "config_events.h"
#include "hts221_sensor.h"
#include "lps22hb_sensor.h"
#include "lsm303agr_magn_sensor.h"
#include "lsm6dsl_sensor.h"

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_INF);

#define BSL_UUID_SERVICE_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BSL_UUID_SENSOR_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)
#define BSL_UUID_CONTROL_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)
#define BSL_UUID_CONFIG_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3)
#define BSL_UUID_STATUS_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef4)
#define BSL_UUID_CAPABILITY_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef5)

static struct bt_uuid_128 service_uuid = BT_UUID_INIT_128(BSL_UUID_SERVICE_VAL);
static struct bt_uuid_128 sensor_uuid = BT_UUID_INIT_128(BSL_UUID_SENSOR_VAL);
static struct bt_uuid_128 control_uuid = BT_UUID_INIT_128(BSL_UUID_CONTROL_VAL);
static struct bt_uuid_128 config_uuid = BT_UUID_INIT_128(BSL_UUID_CONFIG_VAL);
static struct bt_uuid_128 status_uuid = BT_UUID_INIT_128(BSL_UUID_STATUS_VAL);
static struct bt_uuid_128 capability_uuid = BT_UUID_INIT_128(BSL_UUID_CAPABILITY_VAL);

static struct bt_conn *current_conn;
static bool notify_enabled;
static struct bsl_config current_config;
static struct bsl_status current_status;
static struct bsl_capability current_capability;
static struct k_work_delayable advertising_work;
static struct k_work_delayable batch_flush_work;
static struct k_mutex batch_lock;

struct stream_batch_state {
	bool active;
	uint16_t first_sequence;
	uint32_t first_timestamp_ms;
	uint8_t stream_id;
	uint8_t flags;
	uint8_t payload_format;
	uint8_t sample_size;
	uint8_t payload_len;
	uint8_t sample_count;
	uint8_t payload[BSL_SENSOR_DATA_MAX_PAYLOAD_SIZE];
};

static struct stream_batch_state imu6_batch;
static struct stream_batch_state orientation_batch;

static bool stream_supports_batching(uint8_t stream_id)
{
#ifdef CONFIG_BSL_LATENCY_PRIORITY
	ARG_UNUSED(stream_id);
	return false;
#else
	return stream_id == BSL_STREAM_ID_LSM6DSL_IMU6 ||
	       stream_id == BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION;
#endif
}

static uint8_t stream_sample_size(const struct bsl_sensor_data *sample)
{
	switch (sample->header.stream_id) {
	case BSL_STREAM_ID_DUMMY_ACCEL3:
		return BSL_SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE;
	case BSL_STREAM_ID_LSM6DSL_IMU6:
		return BSL_SENSOR_IMU6_SAMPLE_SIZE;
	case BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION:
		return BSL_SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE;
	case BSL_STREAM_ID_LSM303AGR_MAG3:
		return BSL_SENSOR_MAG3_SAMPLE_SIZE;
	case BSL_STREAM_ID_HTS221_TEMP_HUMIDITY:
		return BSL_SENSOR_HTS221_SAMPLE_SIZE;
	case BSL_STREAM_ID_LPS22HB_PRESSURE:
		return BSL_SENSOR_LPS22HB_SAMPLE_SIZE;
	default:
		return 0U;
	}
}

static uint8_t stream_max_batch_samples(uint8_t stream_id)
{
	if (stream_id == BSL_STREAM_ID_LSM6DSL_IMU6) {
		return BSL_SENSOR_BATCH_MAX_SAMPLES_IMU6;
	}
	if (stream_id == BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION) {
		return BSL_SENSOR_BATCH_MAX_SAMPLES_ORIENTATION;
	}
	return 1U;
}

static struct stream_batch_state *batch_state_for_stream(uint8_t stream_id)
{
	if (stream_id == BSL_STREAM_ID_LSM6DSL_IMU6) {
		return &imu6_batch;
	}
	if (stream_id == BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION) {
		return &orientation_batch;
	}
	return NULL;
}

static ssize_t read_config(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, uint16_t len, uint16_t offset)
{
	uint8_t payload[BSL_CONFIG_SIZE];
	int err = bsl_config_pack(&current_config, payload, sizeof(payload));

	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, payload, sizeof(payload));
}

static ssize_t write_config(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, uint16_t len, uint16_t offset,
			    uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	struct bsl_config config;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (bsl_config_unpack(buf, len, &config) != 0) {
		current_status.last_error = BSL_ERROR_INVALID_CONFIG;
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	current_config = config;
	struct config_update_event *event = new_config_update_event();
	event->config = config;
	APP_EVENT_SUBMIT(event);

	return len;
}

static ssize_t write_control(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     const void *buf, uint16_t len, uint16_t offset,
			     uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	struct bsl_control command;
	int err;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	err = bsl_control_unpack(buf, len, &command);
	if (err == -EINVAL) {
		current_status.last_error = BSL_ERROR_INVALID_LENGTH;
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (err == -EPROTO) {
		current_status.last_error = BSL_ERROR_INVALID_VERSION;
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}
	if (err) {
		current_status.last_error = BSL_ERROR_INVALID_COMMAND;
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	struct pc_command_event *event = new_pc_command_event();
	event->command = command;
	APP_EVENT_SUBMIT(event);

	return len;
}

static ssize_t read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, uint16_t len, uint16_t offset)
{
	uint8_t payload[BSL_STATUS_SIZE];
	int err = bsl_status_pack(&current_status, payload, sizeof(payload));

	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, payload, sizeof(payload));
}

static ssize_t read_capability(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len, uint16_t offset)
{
	uint8_t payload[BSL_CAPABILITY_SIZE];
	int err = bsl_capability_pack(&current_capability, payload, sizeof(payload));

	if (err < 0) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, payload, (uint16_t)err);
}

static void sensor_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("sensor notifications %s", notify_enabled ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(sensor_svc,
	BT_GATT_PRIMARY_SERVICE(&service_uuid),
	BT_GATT_CHARACTERISTIC(&sensor_uuid.uuid,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC(sensor_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(&control_uuid.uuid,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE,
			       NULL, write_control, NULL),
	BT_GATT_CHARACTERISTIC(&config_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_config, write_config, NULL),
	BT_GATT_CHARACTERISTIC(&status_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_status, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&capability_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_capability, NULL, NULL)
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BSL_UUID_SERVICE_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void advertising_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err && err != -EALREADY) {
		LOG_ERR("advertising start failed: %d", err);
		k_work_schedule(&advertising_work, K_SECONDS(1));
		return;
	}
	LOG_INF("advertising");
}

static void schedule_advertising(k_timeout_t delay)
{
	k_work_schedule(&advertising_work, delay);
}

static void update_optional_sensor_errors(struct bsl_status *status)
{
	status->lsm6dsl_error = lsm6dsl_sensor_status_error();
	status->hts221_error = hts221_sensor_status_error();
	status->lps22hb_error = lps22hb_sensor_status_error();
	status->lsm303agr_magn_error = lsm303agr_magn_sensor_status_error();
}

static uint16_t first_optional_sensor_error(const struct bsl_status *status)
{
	if (status->lsm6dsl_error != BSL_ERROR_NONE) {
		return status->lsm6dsl_error;
	}
	if (status->hts221_error != BSL_ERROR_NONE) {
		return status->hts221_error;
	}
	if (status->lps22hb_error != BSL_ERROR_NONE) {
		return status->lps22hb_error;
	}
	return status->lsm303agr_magn_error;
}

static int notify_packed_sample(const struct bsl_sensor_data *sample)
{
	uint8_t payload[BSL_SENSOR_DATA_SIZE];
	int err;

	if (!current_conn) {
		current_status.last_error = BSL_ERROR_NOT_CONNECTED;
		return -ENOTCONN;
	}
	if (!notify_enabled) {
		current_status.last_error = BSL_ERROR_NOT_SUBSCRIBED;
		return -EACCES;
	}

	err = bsl_sensor_data_pack(sample, payload, sizeof(payload));
	if (err < 0) {
		return err;
	}

	return bt_gatt_notify(current_conn, &sensor_svc.attrs[2], payload, (uint16_t)err);
}

static int flush_batch_state(struct stream_batch_state *state)
{
	struct bsl_sensor_data frame;

	if (!state || !state->active) {
		return 0;
	}

	memset(&frame, 0, sizeof(frame));
	frame.header.version = BSL_PROTOCOL_VERSION;
	frame.header.message_type = BSL_MESSAGE_TYPE_SENSOR_SAMPLE;
	frame.header.stream_id = state->stream_id;
	frame.header.flags = state->flags;
	frame.header.sequence = state->first_sequence;
	frame.header.timestamp_ms = state->first_timestamp_ms;
	frame.header.payload_format = state->payload_format;
	frame.header.payload_len = state->payload_len;
	frame.header.sample_count = state->sample_count;
	memcpy(frame.payload.raw, state->payload, state->payload_len);

	state->active = false;
	state->payload_len = 0U;
	state->sample_count = 0U;
	return notify_packed_sample(&frame);
}

static int flush_all_batches_locked(void)
{
	int err = 0;
	int flush_err;

	flush_err = flush_batch_state(&imu6_batch);
	if (flush_err && !err) {
		err = flush_err;
	}
	flush_err = flush_batch_state(&orientation_batch);
	if (flush_err && !err) {
		err = flush_err;
	}
	return err;
}

static void batch_flush_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_mutex_lock(&batch_lock, K_FOREVER);
	(void)flush_all_batches_locked();
	k_mutex_unlock(&batch_lock);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("connection failed: %u", err);
		schedule_advertising(K_MSEC(200));
		return;
	}

	current_conn = bt_conn_ref(conn);
	current_status.connection_count++;
	struct ble_connected_event *event = new_ble_connected_event();
	APP_EVENT_SUBMIT(event);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
	notify_enabled = false;
	k_work_cancel_delayable(&batch_flush_work);
	k_mutex_lock(&batch_lock, K_FOREVER);
	imu6_batch.active = false;
	imu6_batch.payload_len = 0U;
	imu6_batch.sample_count = 0U;
	orientation_batch.active = false;
	orientation_batch.payload_len = 0U;
	orientation_batch.sample_count = 0U;
	k_mutex_unlock(&batch_lock);

	struct ble_disconnected_event *event = new_ble_disconnected_event();
	event->reason = reason;
	APP_EVENT_SUBMIT(event);
	schedule_advertising(K_MSEC(200));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int ble_service_init(void)
{
	int err;
	uint8_t stream_count = 1;

	bsl_config_default(&current_config);
	bsl_capability_default(&current_capability);
	if (lsm6dsl_sensor_is_available()) {
		stream_count++;
		stream_count++;
	}
	if (hts221_sensor_is_available()) {
		current_capability.streams[stream_count++] = current_capability.streams[3];
	}
	if (lps22hb_sensor_is_available()) {
		current_capability.streams[stream_count++] = current_capability.streams[4];
	}
	if (lsm303agr_magn_sensor_is_available()) {
		current_capability.streams[stream_count++] = current_capability.streams[5];
	}
	current_capability.header.stream_count = stream_count;
	k_work_init_delayable(&advertising_work, advertising_work_handler);
	k_work_init_delayable(&batch_flush_work, batch_flush_work_handler);
	k_mutex_init(&batch_lock);
	current_status.version = BSL_PROTOCOL_VERSION;
	current_status.state = BSL_DEVICE_STATE_IDLE;
	current_status.sample_interval_ms = current_config.sample_interval_ms;
	update_optional_sensor_errors(&current_status);
	current_status.last_error = first_optional_sensor_error(&current_status);
	current_status.connection_count = 0;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("bt_enable failed: %d", err);
		return err;
	}

	schedule_advertising(K_NO_WAIT);
	return 0;
}

int ble_service_enqueue_sample(const struct bsl_sensor_data *sample)
{
	struct stream_batch_state *state;
	uint8_t sample_size;
	uint8_t max_samples;
	uint16_t expected_sequence;
	int err = 0;
	int flush_err;

	if (!sample) {
		return -EINVAL;
	}
	if (sample->header.sample_count != 1U) {
		return -EINVAL;
	}

	sample_size = stream_sample_size(sample);
	if (sample_size == 0U || sample->header.payload_len != sample_size) {
		return -EINVAL;
	}

	if (!stream_supports_batching(sample->header.stream_id)) {
		return notify_packed_sample(sample);
	}

	state = batch_state_for_stream(sample->header.stream_id);
	if (!state) {
		return notify_packed_sample(sample);
	}

	max_samples = stream_max_batch_samples(sample->header.stream_id);

	k_mutex_lock(&batch_lock, K_FOREVER);
	if (!state->active) {
		state->active = true;
		state->first_sequence = sample->header.sequence;
		state->first_timestamp_ms = sample->header.timestamp_ms;
		state->stream_id = sample->header.stream_id;
		state->flags = sample->header.flags;
		state->payload_format = sample->header.payload_format;
		state->sample_size = sample_size;
		state->payload_len = sample_size;
		state->sample_count = 1U;
		memcpy(state->payload, sample->payload.raw, sample_size);
		k_work_reschedule(&batch_flush_work, K_MSEC(BSL_SENSOR_BATCH_FLUSH_MS));
		k_mutex_unlock(&batch_lock);
		return 0;
	}

	expected_sequence = (uint16_t)(state->first_sequence + state->sample_count);
	if (state->payload_format != sample->header.payload_format ||
	    state->sample_size != sample_size ||
	    sample->header.sequence != expected_sequence ||
	    state->sample_count >= max_samples ||
	    (uint16_t)state->payload_len + sample_size > BSL_SENSOR_DATA_MAX_PAYLOAD_SIZE) {
		flush_err = flush_batch_state(state);
		if (flush_err) {
			err = flush_err;
		}
		state->active = true;
		state->first_sequence = sample->header.sequence;
		state->first_timestamp_ms = sample->header.timestamp_ms;
		state->stream_id = sample->header.stream_id;
		state->flags = sample->header.flags;
		state->payload_format = sample->header.payload_format;
		state->sample_size = sample_size;
		state->payload_len = sample_size;
		state->sample_count = 1U;
		memcpy(state->payload, sample->payload.raw, sample_size);
		k_work_reschedule(&batch_flush_work, K_MSEC(BSL_SENSOR_BATCH_FLUSH_MS));
		k_mutex_unlock(&batch_lock);
		return err;
	}

	memcpy(&state->payload[state->payload_len], sample->payload.raw, sample_size);
	state->payload_len = (uint8_t)(state->payload_len + sample_size);
	state->sample_count++;
	if (state->sample_count >= max_samples) {
		flush_err = flush_batch_state(state);
		if (flush_err) {
			err = flush_err;
		}
	}
	k_work_reschedule(&batch_flush_work, K_MSEC(BSL_SENSOR_BATCH_FLUSH_MS));
	k_mutex_unlock(&batch_lock);
	return err;
}

int ble_service_flush_samples(void)
{
	int err;

	k_work_cancel_delayable(&batch_flush_work);
	k_mutex_lock(&batch_lock, K_FOREVER);
	err = flush_all_batches_locked();
	k_mutex_unlock(&batch_lock);
	return err;
}

void ble_service_set_status(const struct bsl_status *status)
{
	if (status) {
		current_status = *status;
	}
}

void ble_service_set_config(const struct bsl_config *config)
{
	if (config) {
		current_config = *config;
		if (config->op == BSL_CONFIG_OP_SET_STREAM_INTERVAL) {
			current_status.sample_interval_ms = config->sample_interval_ms;
		}
	}
}

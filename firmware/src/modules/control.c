#include "control.h"

#include <app_event_manager.h>
#include <zephyr/logging/log.h>

#include "ble_events.h"
#include "ble_service.h"
#include "config_events.h"
#include "hts221_sensor.h"
#include "lps22hb_sensor.h"
#include "lsm303agr_magn_sensor.h"
#include "lsm6dsl_sensor.h"
#include "sensor_dummy.h"
#include "sensor_events.h"
#include "status_events.h"

LOG_MODULE_REGISTER(control, LOG_LEVEL_INF);

static struct bsl_status status = {
	.version = BSL_PROTOCOL_VERSION,
	.state = BSL_DEVICE_STATE_IDLE,
	.sample_interval_ms = BSL_INTERVAL_DEFAULT_MS,
	.last_error = BSL_ERROR_NONE,
	.connection_count = 0,
	.lsm6dsl_error = BSL_ERROR_NONE,
	.hts221_error = BSL_ERROR_NONE,
	.lps22hb_error = BSL_ERROR_NONE,
	.lsm303agr_magn_error = BSL_ERROR_NONE,
};

static void update_optional_sensor_errors(void)
{
	status.lsm6dsl_error = lsm6dsl_sensor_status_error();
	status.hts221_error = hts221_sensor_status_error();
	status.lps22hb_error = lps22hb_sensor_status_error();
	status.lsm303agr_magn_error = lsm303agr_magn_sensor_status_error();
}

static uint16_t first_optional_sensor_error(void)
{
	if (status.lsm6dsl_error != BSL_ERROR_NONE) {
		return status.lsm6dsl_error;
	}
	if (status.hts221_error != BSL_ERROR_NONE) {
		return status.hts221_error;
	}
	if (status.lps22hb_error != BSL_ERROR_NONE) {
		return status.lps22hb_error;
	}
	return status.lsm303agr_magn_error;
}

static void update_optional_sensor_status(void)
{
	update_optional_sensor_errors();
	status.last_error = first_optional_sensor_error();
}

static void publish_status(void)
{
	ble_service_set_status(&status);

	struct status_update_event *event = new_status_update_event();
	event->status = status;
	APP_EVENT_SUBMIT(event);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_pc_command_event(aeh)) {
		const struct pc_command_event *event = cast_pc_command_event(aeh);

		switch (event->command.command) {
		case BSL_COMMAND_START_MEASUREMENT:
			status.state = BSL_DEVICE_STATE_MEASURING;
			update_optional_sensor_status();
			lsm6dsl_sensor_start();
			hts221_sensor_start();
			lps22hb_sensor_start();
			lsm303agr_magn_sensor_start();
			sensor_dummy_start();
			break;
		case BSL_COMMAND_STOP_MEASUREMENT:
			status.state = BSL_DEVICE_STATE_IDLE;
			update_optional_sensor_status();
			lsm6dsl_sensor_stop();
			hts221_sensor_stop();
			lps22hb_sensor_stop();
			lsm303agr_magn_sensor_stop();
			sensor_dummy_stop();
			(void)ble_service_flush_samples();
			break;
		case BSL_COMMAND_REQUEST_STATUS:
			break;
		case BSL_COMMAND_RESET_SEQUENCE:
			lsm6dsl_sensor_reset_sequence();
			hts221_sensor_reset_sequence();
			lps22hb_sensor_reset_sequence();
			lsm303agr_magn_sensor_reset_sequence();
			sensor_dummy_reset_sequence();
			break;
		case BSL_COMMAND_FORCE_GYRO_CALIB:
			lsm6dsl_sensor_force_gyro_calib();
			break;
		default:
			status.last_error = BSL_ERROR_INVALID_COMMAND;
			break;
		}
		publish_status();
		return false;
	}

	if (is_config_update_event(aeh)) {
		const struct config_update_event *event = cast_config_update_event(aeh);

		switch (event->config.op) {
		case BSL_CONFIG_OP_SET_STREAM_INTERVAL:
			status.sample_interval_ms = event->config.sample_interval_ms;
			sensor_dummy_set_interval(event->config.sample_interval_ms);
			break;
		case BSL_CONFIG_OP_SET_COMPLEMENTARY_ALPHA:
			lsm6dsl_sensor_set_complementary_alpha_permille(
				event->config.sample_interval_ms);
			break;
		case BSL_CONFIG_OP_SET_MAHONY_KP:
			lsm6dsl_sensor_set_mahony_kp_milli(event->config.sample_interval_ms);
			break;
		case BSL_CONFIG_OP_SET_MAHONY_KI:
			lsm6dsl_sensor_set_mahony_ki_milli(event->config.sample_interval_ms);
			break;
		case BSL_CONFIG_OP_SET_IIR_CUTOFF_MILLIHZ:
			lsm6dsl_sensor_set_iir_cutoff_millihz(
				event->config.sample_interval_ms);
			break;
		case BSL_CONFIG_OP_SET_GYRO_CALIB_THRESHOLD:
			lsm6dsl_sensor_set_gyro_calib_threshold_mdps(
				event->config.sample_interval_ms);
			break;
		case BSL_CONFIG_OP_SET_GYRO_CALIB_ALPHA:
			lsm6dsl_sensor_set_gyro_calib_alpha_permille(
				event->config.sample_interval_ms);
			break;
		case BSL_CONFIG_OP_SET_GYRO_CALIB_WINDOW:
			lsm6dsl_sensor_set_gyro_calib_window_samples(
				event->config.sample_interval_ms);
			break;
		default:
			status.last_error = BSL_ERROR_INVALID_CONFIG;
			publish_status();
			return false;
		}
		update_optional_sensor_status();
		ble_service_set_config(&event->config);
		publish_status();
		return false;
	}

	if (is_ble_connected_event(aeh)) {
		status.connection_count++;
		publish_status();
		return false;
	}

	if (is_ble_disconnected_event(aeh)) {
		status.state = BSL_DEVICE_STATE_IDLE;
		lsm6dsl_sensor_stop();
		hts221_sensor_stop();
		lps22hb_sensor_stop();
		lsm303agr_magn_sensor_stop();
		sensor_dummy_stop();
		(void)ble_service_flush_samples();
		publish_status();
		return false;
	}

	if (is_sensor_sample_event(aeh)) {
		const struct sensor_sample_event *event = cast_sensor_sample_event(aeh);
		(void)ble_service_enqueue_sample(&event->sample);
		return false;
	}

	return false;
}

int control_init(void)
{
	update_optional_sensor_status();
	publish_status();
	return 0;
}

APP_EVENT_LISTENER(control, app_event_handler);
APP_EVENT_SUBSCRIBE(control, pc_command_event);
APP_EVENT_SUBSCRIBE(control, config_update_event);
APP_EVENT_SUBSCRIBE(control, ble_connected_event);
APP_EVENT_SUBSCRIBE(control, ble_disconnected_event);
APP_EVENT_SUBSCRIBE(control, sensor_sample_event);

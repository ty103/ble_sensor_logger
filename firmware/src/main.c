#include <app_event_manager.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ble_service.h"
#include "control.h"
#include "hts221_sensor.h"
#if IS_ENABLED(CONFIG_BSL_IKS01A2_I2C_PROBE)
#include "iks01a2_i2c_probe.h"
#endif
#include "lps22hb_sensor.h"
#include "lsm303agr_magn_sensor.h"
#include "lsm6dsl_sensor.h"
#include "sensor_dummy.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	int err;

	LOG_INF("BLE Sensor Logger starting");

	err = app_event_manager_init();
	if (err) {
		LOG_ERR("app_event_manager_init failed: %d", err);
		return err;
	}

	err = sensor_dummy_init();
	if (err) {
		LOG_ERR("sensor_dummy_init failed: %d", err);
		return err;
	}

#if IS_ENABLED(CONFIG_BSL_IKS01A2_I2C_PROBE)
	iks01a2_i2c_probe_run();
#endif

	err = lsm6dsl_sensor_init();
	if (err) {
		LOG_ERR("lsm6dsl_sensor_init failed: %d", err);
	}

	err = hts221_sensor_init();
	if (err) {
		LOG_ERR("hts221_sensor_init failed: %d", err);
	}

	err = lps22hb_sensor_init();
	if (err) {
		LOG_ERR("lps22hb_sensor_init failed: %d", err);
	}

	err = lsm303agr_magn_sensor_init();
	if (err) {
		LOG_ERR("lsm303agr_magn_sensor_init failed: %d", err);
	}

	err = control_init();
	if (err) {
		LOG_ERR("control_init failed: %d", err);
		return err;
	}

	err = ble_service_init();
	if (err) {
		LOG_ERR("ble_service_init failed: %d", err);
		return err;
	}

	LOG_INF("BLE Sensor Logger ready");
	return 0;
}

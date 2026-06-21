#include "adc_sensor.h"

#include <errno.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adc_sensor, LOG_LEVEL_INF);

#define ADC_SAMPLE_INTERVAL_MS 100

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "ADC io-channels must be defined under /zephyr,user"
#endif

static const struct adc_dt_spec adc_channel =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static struct k_work_delayable sample_work;
static int16_t latest_raw;
static uint16_t latest_mv;
static bool latest_valid;
static bool running;

static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!running) {
		return;
	}

	int16_t sample_buffer;
	struct adc_sequence sequence = {
		.buffer = &sample_buffer,
		.buffer_size = sizeof(sample_buffer),
	};

	int err = adc_sequence_init_dt(&adc_channel, &sequence);
	if (err == 0) {
		err = adc_read_dt(&adc_channel, &sequence);
	}

	if (err == 0) {
		int32_t millivolts = sample_buffer;

		err = adc_raw_to_millivolts_dt(&adc_channel, &millivolts);
		if (err == 0) {
			latest_raw = sample_buffer;
			latest_mv = (uint16_t)CLAMP(millivolts, 0, UINT16_MAX);
			latest_valid = true;
		} else {
			LOG_WRN("ADC conversion failed: %d", err);
		}
	} else {
		LOG_WRN("ADC read failed: %d", err);
	}

	k_work_schedule(&sample_work, K_MSEC(ADC_SAMPLE_INTERVAL_MS));
}

int adc_sensor_init(void)
{
	if (!adc_is_ready_dt(&adc_channel)) {
		LOG_ERR("ADC controller not ready");
		return -ENODEV;
	}

	int err = adc_channel_setup_dt(&adc_channel);
	if (err) {
		LOG_ERR("ADC channel setup failed: %d", err);
		return err;
	}

	k_work_init_delayable(&sample_work, sample_work_handler);
	LOG_INF("ADC ready: channel=%u, fs=%u Hz", adc_channel.channel_id,
		1000U / ADC_SAMPLE_INTERVAL_MS);
	return 0;
}

void adc_sensor_start(void)
{
	if (running) {
		return;
	}
	running = true;
	k_work_schedule(&sample_work, K_NO_WAIT);
}

void adc_sensor_stop(void)
{
	running = false;
	k_work_cancel_delayable(&sample_work);
}

void adc_sensor_get_latest(int16_t *raw, uint16_t *millivolts, bool *valid)
{
	if (raw) {
		*raw = latest_raw;
	}
	if (millivolts) {
		*millivolts = latest_mv;
	}
	if (valid) {
		*valid = latest_valid;
	}
}

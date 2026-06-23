"""BLE E2E smoke test for a flashed nRF52840 DK.

This is intentionally a manual/integration test helper. It requires a nearby
BLE_SENSOR_LOGGER device and host Bluetooth permissions.
"""

from __future__ import annotations

import argparse
import asyncio
import time

from ble_sensor_logger.app_core import SensorLoggerApp
from ble_sensor_logger.protocol import SensorDataPayload


async def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", default="BLE_SENSOR_LOGGER")
    parser.add_argument("--first-interval-ms", type=int, default=100)
    parser.add_argument("--second-interval-ms", type=int, default=500)
    parser.add_argument("--first-window-s", type=float, default=3.0)
    parser.add_argument("--second-window-s", type=float, default=3.0)
    args = parser.parse_args()

    app = SensorLoggerApp()
    samples: list[tuple[float, SensorDataPayload]] = []

    def on_sample(sample: SensorDataPayload) -> None:
        samples.append((time.monotonic(), sample))

    devices = await app.scan()
    print(f"scan_count={len(devices)}")
    for device in devices:
        print(f"device name={device.name} address={device.address} rssi={device.rssi}")

    await app.connect(args.name)
    print("connected=true")

    capability = await app.read_capability()
    print(
        "capability schema={} flags=0x{:04x} features=0x{:04x} preferred_mtu={} streams={}".format(
            capability.schema_version,
            capability.capability_flags,
            capability.supported_features,
            capability.preferred_mtu,
            len(capability.streams),
        )
    )
    for stream in capability.streams:
        print(
            "capability_stream id={} type={} channels={} format={} interval={}ms range={}..{}ms".format(
                stream.stream_id,
                stream.stream_type.name,
                stream.channel_count,
                stream.payload_format.name,
                stream.default_interval_ms,
                stream.min_interval_ms,
                stream.max_interval_ms,
            )
        )

    await app.set_orientation_filter_params(0.97, 0.6, 0.02, 2.5)
    filter_config = await app.read_config()
    print(
        "orientation_filter_config_last=op:{} stream:{} value:{}".format(
            filter_config.op.name,
            filter_config.stream_id,
            filter_config.sample_interval_ms,
        )
    )

    await app.start_monitoring(on_sample)
    print("monitoring=true")

    await app.set_interval(args.first_interval_ms)
    await app.start_measurement()
    print(f"started interval_ms={args.first_interval_ms}")
    first_start = time.monotonic()
    await asyncio.sleep(args.first_window_s)
    first_samples = [item for item in samples if item[0] >= first_start]
    print(f"first_window_samples={len(first_samples)}")
    for stream_id in sorted({sample.stream_id for _, sample in first_samples}):
        count = sum(1 for _, sample in first_samples if sample.stream_id == stream_id)
        print(f"first_window_stream_{stream_id}_samples={count}")

    await app.set_interval(args.second_interval_ms)
    print(f"interval_changed interval_ms={args.second_interval_ms}")
    second_start = time.monotonic()
    await asyncio.sleep(args.second_window_s)
    second_samples = [item for item in samples if item[0] >= second_start]
    print(f"second_window_samples={len(second_samples)}")
    for stream_id in sorted({sample.stream_id for _, sample in second_samples}):
        count = sum(1 for _, sample in second_samples if sample.stream_id == stream_id)
        print(f"second_window_stream_{stream_id}_samples={count}")

    status = await app.request_status()
    print(
        "status state={} interval_ms={} last_error={} connections={} "
        "optional_sensors=lsm6dsl:{},hts221:{},lps22hb:{},lsm303agr_magn:{}".format(
            status.state.name,
            status.sample_interval_ms,
            status.last_error.name,
            status.connection_count,
            status.lsm6dsl_error.name,
            status.hts221_error.name,
            status.lps22hb_error.name,
            status.lsm303agr_magn_error.name,
        )
    )

    await app.stop_measurement()
    stop_start = time.monotonic()
    await asyncio.sleep(1.0)
    after_stop_samples = [item for item in samples if item[0] >= stop_start]
    print(f"after_stop_samples={len(after_stop_samples)}")

    print(f"missed_samples={app.state.missed_samples}")
    if samples:
        latest = samples[-1][1]
        print(f"latest_stream_id={latest.stream_id}")
        print(f"latest_accel_mg={latest.accel_x_mg},{latest.accel_y_mg},{latest.accel_z_mg}")
        print(f"latest_gyro_mdps={latest.gyro_x_mdps},{latest.gyro_y_mdps},{latest.gyro_z_mdps}")
        print(f"latest_mag_ut={latest.mag_x_ut},{latest.mag_y_ut},{latest.mag_z_ut}")
        print(
            "latest_orientation_cdeg={},{},{},{},{},{},{},{},{},{},{},{},{} accel_norm_mg={}".format(
                latest.pitch_naive_cdeg,
                latest.roll_naive_cdeg,
                latest.zenith_naive_cdeg,
                latest.pitch_iir_cdeg,
                latest.roll_iir_cdeg,
                latest.zenith_iir_cdeg,
                latest.pitch_complementary_cdeg,
                latest.roll_complementary_cdeg,
                latest.zenith_complementary_cdeg,
                latest.pitch_mahony_cdeg,
                latest.roll_mahony_cdeg,
                latest.zenith_mahony_cdeg,
                latest.yaw_mahony_cdeg,
                latest.accel_norm_mg,
            )
        )
        print(
            "latest_hts221_centi={}rh,{}c".format(
                latest.humidity_centi_percent,
                latest.temperature_centi_c,
            )
        )
        print(f"latest_lps22hb_pressure_pa={latest.pressure_pa}")
        dummy_samples = [sample for _, sample in samples if sample.stream_id == 1]
        if dummy_samples:
            latest_dummy = dummy_samples[-1]
            print(
                "latest_dummy_accel_mg={},{},{}".format(
                    latest_dummy.accel_x_mg,
                    latest_dummy.accel_y_mg,
                    latest_dummy.accel_z_mg,
                )
            )
        hts221_samples = [sample for _, sample in samples if sample.stream_id == 30]
        if hts221_samples:
            latest_hts221 = hts221_samples[-1]
            print(f"latest_humidity_percent={latest_hts221.humidity_centi_percent / 100:.2f}")
            print(f"latest_temperature_c={latest_hts221.temperature_centi_c / 100:.2f}")
        lps22hb_samples = [sample for _, sample in samples if sample.stream_id == 20]
        if lps22hb_samples:
            latest_lps22hb = lps22hb_samples[-1]
            print(f"latest_pressure_pa={latest_lps22hb.pressure_pa}")
        mag_samples = [sample for _, sample in samples if sample.stream_id == 12]
        if mag_samples:
            latest_mag = mag_samples[-1]
            print(f"latest_magnetometer_ut={latest_mag.mag_x_ut},{latest_mag.mag_y_ut},{latest_mag.mag_z_ut}")
        orientation_samples = [sample for _, sample in samples if sample.stream_id == 13]
        if orientation_samples:
            latest_orientation = orientation_samples[-1]
            print(
                "latest_orientation_deg={:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f} "
                "accel_norm_mg={}".format(
                    latest_orientation.pitch_naive_cdeg / 100,
                    latest_orientation.roll_naive_cdeg / 100,
                    latest_orientation.zenith_naive_cdeg / 100,
                    latest_orientation.pitch_iir_cdeg / 100,
                    latest_orientation.roll_iir_cdeg / 100,
                    latest_orientation.zenith_iir_cdeg / 100,
                    latest_orientation.pitch_complementary_cdeg / 100,
                    latest_orientation.roll_complementary_cdeg / 100,
                    latest_orientation.zenith_complementary_cdeg / 100,
                    latest_orientation.pitch_mahony_cdeg / 100,
                    latest_orientation.roll_mahony_cdeg / 100,
                    latest_orientation.zenith_mahony_cdeg / 100,
                    latest_orientation.yaw_mahony_cdeg / 100,
                    latest_orientation.accel_norm_mg,
                )
            )
    await app.disconnect()
    print("disconnected=true")


if __name__ == "__main__":
    asyncio.run(main())

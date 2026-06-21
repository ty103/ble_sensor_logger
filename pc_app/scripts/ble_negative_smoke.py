"""BLE negative smoke test for malformed writes."""

from __future__ import annotations

import asyncio

from ble_sensor_logger.app_core import SensorLoggerApp
from ble_sensor_logger.protocol import (
    CONFIG_FORMAT,
    CONFIG_VERSION,
    STATUS_SIZE,
    ConfigOp,
    DeviceError,
    StatusPayload,
)
import struct


async def main() -> None:
    app = SensorLoggerApp()
    await app.connect("BLE_SENSOR_LOGGER")
    print("connected=true")

    invalid_config = struct.pack(
        CONFIG_FORMAT,
        CONFIG_VERSION,
        int(ConfigOp.SET_STREAM_INTERVAL),
        1,
        0,
        10,
        0,
    )
    try:
        await app.client.write_config(invalid_config)
        print("invalid_config_write=accepted")
    except Exception as exc:
        print(f"invalid_config_write=rejected type={type(exc).__name__}")

    status_raw = await app.client.read_status()
    print(f"status_len={len(status_raw)} expected={STATUS_SIZE}")
    status = StatusPayload.unpack(status_raw)
    config_status = status
    print(
        "status_after_invalid_config state={} interval_ms={} last_error={}".format(
            status.state.name,
            status.sample_interval_ms,
            status.last_error.name,
        )
    )

    try:
        await app.client.write_control(b"\x01")
        print("invalid_control_write=accepted")
    except Exception as exc:
        print(f"invalid_control_write=rejected type={type(exc).__name__}")

    status = StatusPayload.unpack(await app.client.read_status())
    print(
        "status_after_invalid_control state={} interval_ms={} last_error={}".format(
            status.state.name,
            status.sample_interval_ms,
            status.last_error.name,
        )
    )

    print(
        "invalid_config_error_seen={}".format(
            config_status.last_error in {DeviceError.INVALID_LENGTH, DeviceError.INVALID_CONFIG}
        )
    )
    await app.disconnect()
    print("disconnected=true")


if __name__ == "__main__":
    asyncio.run(main())

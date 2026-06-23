"""Application core shared by the CUI and WebGUI backend."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass, field

from .ble_client import BleSensorClient, DiscoveredDevice
from .protocol import (
    Command,
    CapabilityPayload,
    ConfigPayload,
    ConfigOp,
    ControlPayload,
    CONFIG_VERSION,
    STREAM_ID_DUMMY_ACCEL3,
    STREAM_ID_LSM6DSL_ORIENTATION_MOTION,
    SensorDataPayload,
    StatusPayload,
)


SampleHandler = Callable[[SensorDataPayload], None]
DisconnectHandler = Callable[[], None]


@dataclass
class AppState:
    connected: bool = False
    monitoring: bool = False
    last_sequence: int | None = None
    last_sequences_by_stream: dict[int, int] = field(default_factory=dict)
    missed_samples: int = 0


class SensorLoggerApp:
    """UI-independent application use cases."""

    def __init__(self, client: BleSensorClient | None = None):
        self.client = client or BleSensorClient()
        self.state = AppState()
        self._sample_handler: SampleHandler | None = None
        self._disconnect_handler: DisconnectHandler | None = None
        self.client.set_disconnect_handler(self._on_disconnect)

    async def scan(self) -> list[DiscoveredDevice]:
        return await self.client.scan()

    async def connect(self, name_or_address: str | None = None) -> None:
        await self.client.connect(name_or_address)
        self.state.connected = True

    async def disconnect(self) -> None:
        await self.client.disconnect()
        self.state.connected = False
        self.state.monitoring = False

    async def start_monitoring(self, handler: SampleHandler) -> None:
        self._sample_handler = handler
        await self.client.start_notifications(self._handle_sensor_data)
        self.state.monitoring = True

    async def stop_monitoring(self) -> None:
        await self.client.stop_notifications()
        self.state.monitoring = False

    async def start_measurement(self) -> None:
        await self._write_command(Command.START_MEASUREMENT)

    async def stop_measurement(self) -> None:
        await self._write_command(Command.STOP_MEASUREMENT)

    async def reset_sequence(self) -> None:
        await self._write_command(Command.RESET_SEQUENCE)
        self.state.last_sequence = None
        self.state.last_sequences_by_stream.clear()
        self.state.missed_samples = 0

    async def request_status(self) -> StatusPayload:
        await self._write_command(Command.REQUEST_STATUS)
        return await self.read_status()

    async def read_status(self) -> StatusPayload:
        return StatusPayload.unpack(await self.client.read_status())

    async def read_config(self) -> ConfigPayload:
        return ConfigPayload.unpack(await self.client.read_config())

    async def read_capability(self) -> CapabilityPayload:
        return CapabilityPayload.unpack(await self.client.read_capability())

    async def set_interval(self, interval_ms: int) -> None:
        await self.set_stream_interval(STREAM_ID_DUMMY_ACCEL3, interval_ms)

    async def set_stream_interval(self, stream_id: int, interval_ms: int) -> None:
        payload = ConfigPayload(
            version=CONFIG_VERSION,
            op=ConfigOp.SET_STREAM_INTERVAL,
            stream_id=stream_id,
            flags=0,
            sample_interval_ms=interval_ms,
            reserved=0,
        )
        await self.client.write_config(payload.pack())

    async def set_orientation_filter_params(
        self,
        complementary_alpha: float,
        mahony_kp: float,
        mahony_ki: float,
    ) -> None:
        await self._write_orientation_filter_config(
            ConfigOp.SET_COMPLEMENTARY_ALPHA,
            round(complementary_alpha * 1000),
        )
        await self._write_orientation_filter_config(ConfigOp.SET_MAHONY_KP, round(mahony_kp * 1000))
        await self._write_orientation_filter_config(ConfigOp.SET_MAHONY_KI, round(mahony_ki * 1000))

    async def _write_orientation_filter_config(self, op: ConfigOp, value_milli: int) -> None:
        payload = ConfigPayload(
            version=CONFIG_VERSION,
            op=op,
            stream_id=STREAM_ID_LSM6DSL_ORIENTATION_MOTION,
            flags=0,
            sample_interval_ms=value_milli,
            reserved=0,
        )
        await self.client.write_config(payload.pack())

    def set_disconnect_handler(self, handler: DisconnectHandler | None) -> None:
        self._disconnect_handler = handler

    async def _write_command(self, command: Command) -> None:
        await self.client.write_control(ControlPayload.from_command(command).pack())

    def _handle_sensor_data(self, data: bytes) -> None:
        sample = SensorDataPayload.unpack(data)
        self._update_sequence_stats(sample.stream_id, sample.sequence)
        if self._sample_handler is not None:
            self._sample_handler(sample)

    def _update_sequence_stats(self, stream_id: int, sequence: int) -> None:
        last_sequence = self.state.last_sequences_by_stream.get(stream_id)
        if last_sequence is not None:
            expected = (last_sequence + 1) & 0xFFFF
            if sequence != expected:
                self.state.missed_samples += (sequence - expected) & 0xFFFF
        self.state.last_sequences_by_stream[stream_id] = sequence
        self.state.last_sequence = sequence

    def _on_disconnect(self) -> None:
        self.state.connected = False
        self.state.monitoring = False
        if self._disconnect_handler is not None:
            self._disconnect_handler()

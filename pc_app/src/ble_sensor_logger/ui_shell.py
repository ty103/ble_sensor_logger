"""Interactive terminal shell for the PC application."""

from __future__ import annotations

import asyncio
import shlex

from .app_core import SensorLoggerApp
from .ble_client import BleClientError
from .protocol import PayloadFormat, ProtocolError, SensorDataPayload


PROMPT = "ble-sensor> "


class InteractiveShell:
    def __init__(self, app: SensorLoggerApp | None = None):
        self.app = app or SensorLoggerApp()
        self.app.set_disconnect_handler(lambda: print("\nDisconnected"))
        self._running = True

    async def run(self) -> None:
        print("BLE Sensor Logger shell. Type `help` for commands.")
        while self._running:
            try:
                line = await asyncio.to_thread(input, PROMPT)
            except (EOFError, KeyboardInterrupt):
                print()
                break
            await self.dispatch(line)

    async def dispatch(self, line: str) -> None:
        parts = shlex.split(line)
        if not parts:
            return
        command, *args = parts
        try:
            await self._dispatch(command, args)
        except (BleClientError, ProtocolError, ValueError) as exc:
            print(f"error: {exc}")

    async def _dispatch(self, command: str, args: list[str]) -> None:
        match command:
            case "help":
                self._print_help()
            case "scan":
                devices = await self.app.scan()
                if not devices:
                    print("No matching devices found.")
                for device in devices:
                    rssi = "" if device.rssi is None else f" rssi={device.rssi}"
                    print(f"{device.name} {device.address}{rssi}")
            case "connect":
                await self.app.connect(args[0] if args else None)
                print("connected")
            case "disconnect":
                await self.app.disconnect()
                print("disconnected")
            case "monitor":
                await self.app.start_monitoring(self._print_sample)
                print("monitoring")
            case "unmonitor":
                await self.app.stop_monitoring()
                print("monitoring stopped")
            case "start":
                await self.app.start_measurement()
                print("measurement started")
            case "stop":
                await self.app.stop_measurement()
                print("measurement stopped")
            case "set-interval":
                if not args:
                    raise ValueError("usage: set-interval <ms>")
                await self.app.set_interval(int(args[0]))
                print(f"stream 1 interval set to {int(args[0])} ms")
            case "set-stream-interval":
                if len(args) != 2:
                    raise ValueError("usage: set-stream-interval <stream_id> <ms>")
                stream_id = int(args[0])
                interval_ms = int(args[1])
                await self.app.set_stream_interval(stream_id, interval_ms)
                print(f"stream {stream_id} interval set to {interval_ms} ms")
            case "status":
                status = await self.app.read_status()
                print(
                    "state={} interval={}ms last_error={} connections={} "
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
            case "capability":
                capability = await self.app.read_capability()
                print(
                    "schema={} flags=0x{:04x} features=0x{:04x} preferred_mtu={}".format(
                        capability.schema_version,
                        capability.capability_flags,
                        capability.supported_features,
                        capability.preferred_mtu,
                    )
                )
                for stream in capability.streams:
                    print(
                        "stream={} type={} channels={} format={} interval={}ms range={}..{}ms".format(
                            stream.stream_id,
                            stream.stream_type.name,
                            stream.channel_count,
                            stream.payload_format.name,
                            stream.default_interval_ms,
                            stream.min_interval_ms,
                            stream.max_interval_ms,
                        )
                    )
            case "reset-seq":
                await self.app.reset_sequence()
                print("sequence reset")
            case "exit" | "quit":
                await self.app.disconnect()
                self._running = False
            case _:
                print(f"unknown command: {command}")

    def _print_sample(self, sample: SensorDataPayload) -> None:
        if sample.payload_format == PayloadFormat.MAG3_INT16_V1:
            print(
                "stream={} seq={:5d} timestamp={:8d}ms mag=({:5d},{:5d},{:5d})uT missed={}".format(
                    sample.stream_id,
                    sample.sequence,
                    sample.timestamp_ms,
                    sample.mag_x_ut,
                    sample.mag_y_ut,
                    sample.mag_z_ut,
                    self.app.state.missed_samples,
                )
            )
            return

        if sample.payload_format == PayloadFormat.LPS22HB_PRESSURE_INT32_V1:
            print(
                "stream={} seq={:5d} timestamp={:8d}ms pressure={:7.1f}Pa missed={}".format(
                    sample.stream_id,
                    sample.sequence,
                    sample.timestamp_ms,
                    sample.pressure_pa,
                    self.app.state.missed_samples,
                )
            )
            return

        if sample.payload_format == PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1:
            print(
                "stream={} seq={:5d} timestamp={:8d}ms humidity={:5.2f}% temperature={:5.2f}C "
                "missed={}".format(
                    sample.stream_id,
                    sample.sequence,
                    sample.timestamp_ms,
                    sample.humidity_centi_percent / 100,
                    sample.temperature_centi_c / 100,
                    self.app.state.missed_samples,
                )
            )
            return

        if sample.payload_format == PayloadFormat.IMU6_INT16_V1:
            print(
                "stream={} seq={:5d} timestamp={:8d}ms accel=({:5d},{:5d},{:5d})mg "
                "gyro=({:6d},{:6d},{:6d})mdps missed={}".format(
                    sample.stream_id,
                    sample.sequence,
                    sample.timestamp_ms,
                    sample.accel_x_mg,
                    sample.accel_y_mg,
                    sample.accel_z_mg,
                    sample.gyro_x_mdps,
                    sample.gyro_y_mdps,
                    sample.gyro_z_mdps,
                    self.app.state.missed_samples,
                )
            )
            return

        print(
            "stream={} seq={:5d} timestamp={:8d}ms dummy_accel=({:5d},{:5d},{:5d})mg "
            "missed={}".format(
                sample.stream_id,
                sample.sequence,
                sample.timestamp_ms,
                sample.accel_x_mg,
                sample.accel_y_mg,
                sample.accel_z_mg,
                self.app.state.missed_samples,
            )
        )

    @staticmethod
    def _print_help() -> None:
        print(
            "\n".join(
                [
                    "scan",
                    "connect [name_or_address]",
                    "disconnect",
                    "monitor",
                    "unmonitor",
                    "start",
                    "stop",
                    "set-interval <ms>",
                    "set-stream-interval <stream_id> <ms>",
                    "status",
                    "capability",
                    "reset-seq",
                    "exit",
                ]
            )
        )

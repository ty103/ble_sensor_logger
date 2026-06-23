"""Local WebGUI backend.

BLE remains exclusively in this Python process. The browser communicates with
the backend through HTTP and WebSocket APIs.
"""

from __future__ import annotations

import asyncio
from contextlib import suppress
from dataclasses import asdict
import json
from pathlib import Path
import time
from typing import Any

from aiohttp import WSMsgType, web

from .app_core import SensorLoggerApp
from .constants import DEVICE_NAME
from .protocol import CapabilityPayload, PayloadFormat, SensorDataPayload


SAMPLE_VALUE_FIELDS = (
    "accel_x_mg",
    "accel_y_mg",
    "accel_z_mg",
    "gyro_x_mdps",
    "gyro_y_mdps",
    "gyro_z_mdps",
    "humidity_centi_percent",
    "temperature_centi_c",
    "pressure_pa",
    "mag_x_ut",
    "mag_y_ut",
    "mag_z_ut",
    "pitch_naive_cdeg",
    "roll_naive_cdeg",
    "zenith_naive_cdeg",
    "pitch_complementary_cdeg",
    "roll_complementary_cdeg",
    "zenith_complementary_cdeg",
    "pitch_mahony_cdeg",
    "roll_mahony_cdeg",
    "zenith_mahony_cdeg",
    "yaw_mahony_cdeg",
    "accel_norm_mg",
)
FIELDS_BY_PAYLOAD_FORMAT = {
    PayloadFormat.DUMMY_ACCEL3_INT16_V1: (
        "accel_x_mg",
        "accel_y_mg",
        "accel_z_mg",
    ),
    PayloadFormat.IMU6_INT16_V1: (
        "accel_x_mg",
        "accel_y_mg",
        "accel_z_mg",
        "gyro_x_mdps",
        "gyro_y_mdps",
        "gyro_z_mdps",
    ),
    PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1: (
        "humidity_centi_percent",
        "temperature_centi_c",
    ),
    PayloadFormat.LPS22HB_PRESSURE_INT32_V1: ("pressure_pa",),
    PayloadFormat.MAG3_INT16_V1: ("mag_x_ut", "mag_y_ut", "mag_z_ut"),
    PayloadFormat.ORIENTATION_MOTION_INT16_V1: (
        "pitch_naive_cdeg",
        "roll_naive_cdeg",
        "zenith_naive_cdeg",
        "pitch_complementary_cdeg",
        "roll_complementary_cdeg",
        "zenith_complementary_cdeg",
        "pitch_mahony_cdeg",
        "roll_mahony_cdeg",
        "zenith_mahony_cdeg",
        "yaw_mahony_cdeg",
        "accel_norm_mg",
    ),
}
FIELD_METADATA_BY_PAYLOAD_FORMAT = {
    PayloadFormat.DUMMY_ACCEL3_INT16_V1: (
        {
            "field": "accel_x_mg",
            "label": "Dummy Accel X",
            "unit": "mg",
            "scale": 1,
            "decimals": 0,
        },
        {
            "field": "accel_y_mg",
            "label": "Dummy Accel Y",
            "unit": "mg",
            "scale": 1,
            "decimals": 0,
        },
        {
            "field": "accel_z_mg",
            "label": "Dummy Accel Z",
            "unit": "mg",
            "scale": 1,
            "decimals": 0,
        },
    ),
    PayloadFormat.IMU6_INT16_V1: (
        {
            "field": "accel_x_mg",
            "label": "LSM6DSL Accel X",
            "unit": "mg",
            "scale": 1,
            "decimals": 0,
        },
        {
            "field": "accel_y_mg",
            "label": "LSM6DSL Accel Y",
            "unit": "mg",
            "scale": 1,
            "decimals": 0,
        },
        {
            "field": "accel_z_mg",
            "label": "LSM6DSL Accel Z",
            "unit": "mg",
            "scale": 1,
            "decimals": 0,
        },
        {
            "field": "gyro_x_mdps",
            "label": "LSM6DSL Gyro X",
            "unit": "mdps",
            "scale": 1,
            "decimals": 0,
        },
        {
            "field": "gyro_y_mdps",
            "label": "LSM6DSL Gyro Y",
            "unit": "mdps",
            "scale": 1,
            "decimals": 0,
        },
        {
            "field": "gyro_z_mdps",
            "label": "LSM6DSL Gyro Z",
            "unit": "mdps",
            "scale": 1,
            "decimals": 0,
        },
    ),
    PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1: (
        {
            "field": "humidity_centi_percent",
            "label": "HTS221 Humidity",
            "unit": "%RH",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "temperature_centi_c",
            "label": "HTS221 Temperature",
            "unit": "degC",
            "scale": 0.01,
            "decimals": 2,
        },
    ),
    PayloadFormat.LPS22HB_PRESSURE_INT32_V1: (
        {
            "field": "pressure_pa",
            "label": "LPS22HB Pressure",
            "unit": "Pa",
            "scale": 1,
            "decimals": 0,
        },
    ),
    PayloadFormat.MAG3_INT16_V1: (
        {
            "field": "mag_x_ut",
            "label": "LSM303AGR Mag X",
            "unit": "uT",
            "scale": 1,
            "decimals": 0,
        },
        {
            "field": "mag_y_ut",
            "label": "LSM303AGR Mag Y",
            "unit": "uT",
            "scale": 1,
            "decimals": 0,
        },
        {
            "field": "mag_z_ut",
            "label": "LSM303AGR Mag Z",
            "unit": "uT",
            "scale": 1,
            "decimals": 0,
        },
    ),
    PayloadFormat.ORIENTATION_MOTION_INT16_V1: (
        {
            "field": "pitch_naive_cdeg",
            "label": "LSM6DSL Pitch Naive",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "roll_naive_cdeg",
            "label": "LSM6DSL Roll Naive",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "zenith_naive_cdeg",
            "label": "LSM6DSL Zenith Naive",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "pitch_complementary_cdeg",
            "label": "LSM6DSL Pitch Complementary",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "roll_complementary_cdeg",
            "label": "LSM6DSL Roll Complementary",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "zenith_complementary_cdeg",
            "label": "LSM6DSL Zenith Complementary",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "pitch_mahony_cdeg",
            "label": "LSM6DSL Pitch Mahony",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "roll_mahony_cdeg",
            "label": "LSM6DSL Roll Mahony",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "zenith_mahony_cdeg",
            "label": "LSM6DSL Zenith Mahony",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "yaw_mahony_cdeg",
            "label": "LSM6DSL Yaw Mahony",
            "unit": "degree",
            "scale": 0.01,
            "decimals": 2,
        },
        {
            "field": "accel_norm_mg",
            "label": "LSM6DSL Accel Norm",
            "unit": "mg",
            "scale": 1,
            "decimals": 0,
        },
    ),
}


class WebBackend:
    def __init__(self, app: SensorLoggerApp | None = None):
        self.sensor_app = app or SensorLoggerApp()
        self.sensor_app.set_disconnect_handler(self._on_disconnect)
        self.websockets: set[web.WebSocketResponse] = set()
        self.latest_sample: dict[str, Any] | None = None
        self.loop: asyncio.AbstractEventLoop | None = None
        self.connecting = False
        self.connection_task: asyncio.Task[None] | None = None

    async def startup(self, _app: web.Application) -> None:
        self.loop = asyncio.get_running_loop()

    async def shutdown(self, _app: web.Application) -> None:
        await self._cancel_connection()
        if self.sensor_app.state.connected:
            await self.sensor_app.disconnect()
        for websocket in list(self.websockets):
            await websocket.close()

    async def scan(self, _request: web.Request) -> web.Response:
        devices = await self.sensor_app.scan()
        return web.json_response({"devices": [asdict(device) for device in devices]})

    async def connect(self, request: web.Request) -> web.Response:
        if self.connection_task is not None and not self.connection_task.done():
            raise web.HTTPConflict(text="connection already in progress")

        body = await request.json()
        target = body.get("target") or DEVICE_NAME
        self.connecting = True
        await self.broadcast_state()
        task = asyncio.create_task(self._connect(target))
        self.connection_task = task
        try:
            await task
            return web.json_response({"ok": True, "target": target})
        except asyncio.CancelledError:
            return web.json_response({"ok": False, "cancelled": True})
        except Exception as exc:
            return web.json_response({"ok": False, "error": str(exc)}, status=502)
        finally:
            if self.connection_task is task:
                self.connection_task = None
            self.connecting = False
            await self.broadcast_state()

    async def cancel_connect(self, _request: web.Request) -> web.Response:
        cancelled = await self._cancel_connection()
        await self.broadcast_state()
        return web.json_response({"ok": True, "cancelled": cancelled})

    async def disconnect(self, _request: web.Request) -> web.Response:
        await self.sensor_app.disconnect()
        await self.broadcast_state()
        return web.json_response({"ok": True})

    async def start(self, _request: web.Request) -> web.Response:
        await self.sensor_app.start_measurement()
        await self.broadcast_state()
        return web.json_response({"ok": True})

    async def stop(self, _request: web.Request) -> web.Response:
        await self.sensor_app.stop_measurement()
        await self.broadcast_state()
        return web.json_response({"ok": True})

    async def reset_sequence(self, _request: web.Request) -> web.Response:
        await self.sensor_app.reset_sequence()
        await self.broadcast_state()
        return web.json_response({"ok": True})

    async def set_interval(self, request: web.Request) -> web.Response:
        body = await request.json()
        stream_id = int(body.get("stream_id", 1))
        interval_ms = int(body["interval_ms"])
        await self.sensor_app.set_stream_interval(stream_id, interval_ms)
        await self.broadcast_state()
        return web.json_response({"ok": True, "stream_id": stream_id, "interval_ms": interval_ms})

    async def set_orientation_filter(self, request: web.Request) -> web.Response:
        body = await request.json()
        complementary_alpha = float(body["complementary_alpha"])
        mahony_kp = float(body["mahony_kp"])
        mahony_ki = float(body["mahony_ki"])
        await self.sensor_app.set_orientation_filter_params(
            complementary_alpha,
            mahony_kp,
            mahony_ki,
        )
        await self.broadcast_state()
        return web.json_response(
            {
                "ok": True,
                "complementary_alpha": complementary_alpha,
                "mahony_kp": mahony_kp,
                "mahony_ki": mahony_ki,
            }
        )

    async def status(self, _request: web.Request) -> web.Response:
        status = None
        if self.sensor_app.state.connected:
            payload = await self.sensor_app.read_status()
            status = {
                "state": payload.state.name,
                "sample_interval_ms": payload.sample_interval_ms,
                "last_error": payload.last_error.name,
                "connection_count": payload.connection_count,
                "optional_sensors": {
                    "lsm6dsl": {"error": payload.lsm6dsl_error.name},
                    "hts221": {"error": payload.hts221_error.name},
                    "lps22hb": {"error": payload.lps22hb_error.name},
                    "lsm303agr_magn": {"error": payload.lsm303agr_magn_error.name},
                },
            }
        return web.json_response(
            {
                "connected": self.sensor_app.state.connected,
                "connecting": self.connecting,
                "monitoring": self.sensor_app.state.monitoring,
                "missed_samples": self.sensor_app.state.missed_samples,
                "status": status,
                "latest_sample": self.latest_sample,
            }
        )

    async def capability(self, _request: web.Request) -> web.Response:
        if not self.sensor_app.state.connected:
            raise web.HTTPConflict(text="not connected")
        payload = await self.sensor_app.read_capability()
        return web.json_response({"capability": self._capability_to_dict(payload)})

    async def websocket(self, request: web.Request) -> web.WebSocketResponse:
        websocket = web.WebSocketResponse(heartbeat=20)
        await websocket.prepare(request)
        self.websockets.add(websocket)
        await websocket.send_json(self.state_message())
        if self.latest_sample is not None:
            await websocket.send_json({"type": "sample", "sample": self.latest_sample})
        try:
            async for message in websocket:
                if message.type == WSMsgType.ERROR:
                    break
        finally:
            self.websockets.discard(websocket)
        return websocket

    def _on_sample(self, sample: SensorDataPayload) -> None:
        self.latest_sample = {
            **self._sample_to_dict(sample),
            "host_time_ms": round(time.time() * 1000),
            "missed_samples": self.sensor_app.state.missed_samples,
        }
        self._schedule_broadcast({"type": "sample", "sample": self.latest_sample})

    def _sample_to_dict(self, sample: SensorDataPayload) -> dict[str, Any]:
        payload = asdict(sample)
        active_fields = set(FIELDS_BY_PAYLOAD_FORMAT[sample.payload_format])
        for field in SAMPLE_VALUE_FIELDS:
            if field not in active_fields:
                payload[field] = None
        return payload

    def _on_disconnect(self) -> None:
        self._schedule_broadcast(self.state_message())

    def state_message(self) -> dict[str, Any]:
        return {
            "type": "state",
            "connected": self.sensor_app.state.connected,
            "connecting": self.connecting,
            "monitoring": self.sensor_app.state.monitoring,
            "missed_samples": self.sensor_app.state.missed_samples,
        }

    def _capability_to_dict(self, payload: CapabilityPayload) -> dict[str, Any]:
        return {
            "version": payload.version,
            "schema_version": payload.schema_version,
            "capability_flags": payload.capability_flags,
            "supported_commands": payload.supported_commands,
            "supported_features": payload.supported_features,
            "preferred_mtu": payload.preferred_mtu,
            "streams": [
                {
                    "stream_id": stream.stream_id,
                    "stream_type": stream.stream_type.name,
                    "channel_count": stream.channel_count,
                    "data_type": stream.data_type.name,
                    "unit": stream.unit.name,
                    "payload_format": stream.payload_format.name,
                    "stream_flags": stream.stream_flags,
                    "default_interval_ms": stream.default_interval_ms,
                    "min_interval_ms": stream.min_interval_ms,
                    "max_interval_ms": stream.max_interval_ms,
                    "scale_exponent": stream.scale_exponent,
                    "fields": list(FIELD_METADATA_BY_PAYLOAD_FORMAT[stream.payload_format]),
                }
                for stream in payload.streams
            ],
        }

    async def _connect(self, target: str) -> None:
        try:
            await self.sensor_app.connect(target)
            if not self.sensor_app.state.monitoring:
                await self.sensor_app.start_monitoring(self._on_sample)
        except BaseException:
            await self.sensor_app.disconnect()
            raise

    async def _cancel_connection(self) -> bool:
        task = self.connection_task
        if task is None or task.done():
            return False

        cancelled = task.cancel()
        with suppress(asyncio.CancelledError):
            await task
        if cancelled:
            await self.sensor_app.disconnect()
        return cancelled

    async def broadcast_state(self) -> None:
        await self.broadcast(self.state_message())

    async def broadcast(self, message: dict[str, Any]) -> None:
        if not self.websockets:
            return
        payload = json.dumps(message)
        stale: list[web.WebSocketResponse] = []
        for websocket in self.websockets:
            try:
                await websocket.send_str(payload)
            except ConnectionError:
                stale.append(websocket)
        for websocket in stale:
            self.websockets.discard(websocket)

    def _schedule_broadcast(self, message: dict[str, Any]) -> None:
        if self.loop is not None and self.loop.is_running():
            self.loop.create_task(self.broadcast(message))


BACKEND_KEY = web.AppKey("backend", WebBackend)


def create_web_app(sensor_app: SensorLoggerApp | None = None) -> web.Application:
    backend = WebBackend(sensor_app)
    web_root = Path(__file__).resolve().parents[2] / "web_frontend"

    app = web.Application()
    app[BACKEND_KEY] = backend
    app.on_startup.append(backend.startup)
    app.on_shutdown.append(backend.shutdown)
    app.router.add_get("/api/scan", backend.scan)
    app.router.add_post("/api/connect", backend.connect)
    app.router.add_post("/api/connect/cancel", backend.cancel_connect)
    app.router.add_post("/api/disconnect", backend.disconnect)
    app.router.add_post("/api/start", backend.start)
    app.router.add_post("/api/stop", backend.stop)
    app.router.add_post("/api/reset-sequence", backend.reset_sequence)
    app.router.add_post("/api/interval", backend.set_interval)
    app.router.add_post("/api/orientation-filter", backend.set_orientation_filter)
    app.router.add_get("/api/status", backend.status)
    app.router.add_get("/api/capability", backend.capability)
    app.router.add_get("/ws", backend.websocket)

    async def index(_request: web.Request) -> web.FileResponse:
        return web.FileResponse(web_root / "index.html")

    app.router.add_get("/", index)
    app.router.add_static("/static", web_root, show_index=False)
    return app


def run_web_server(host: str = "127.0.0.1", port: int = 8765) -> None:
    web.run_app(create_web_app(), host=host, port=port, print=None, handle_signals=False)

"""BLE client wrapper around bleak.

This module is intentionally UI-neutral. The WebGUI backend exposes
this through app_core/web_api instead of moving BLE into the browser.
"""

from __future__ import annotations

import asyncio
from collections.abc import Awaitable, Callable
from dataclasses import dataclass
from typing import Any

from .constants import (
    CAPABILITY_UUID,
    CONFIG_UUID,
    CONTROL_UUID,
    DEFAULT_SCAN_TIMEOUT_S,
    DEVICE_NAME,
    SENSOR_DATA_UUID,
    SERVICE_UUID,
    STATUS_UUID,
)


NotificationHandler = Callable[[bytes], None]
DisconnectHandler = Callable[[], None]


@dataclass(frozen=True)
class DiscoveredDevice:
    name: str
    address: str
    rssi: int | None = None


class BleClientError(RuntimeError):
    """Raised for BLE client operation failures."""


class BleSensorClient:
    """UI-agnostic BLE central client."""

    def __init__(self, name: str = DEVICE_NAME, scan_timeout_s: float = DEFAULT_SCAN_TIMEOUT_S):
        self.name = name
        self.scan_timeout_s = scan_timeout_s
        self._client: Any | None = None
        self._address: str | None = None
        self._notification_handler: NotificationHandler | None = None
        self._disconnect_handler: DisconnectHandler | None = None

    @property
    def is_connected(self) -> bool:
        return bool(self._client and self._client.is_connected)

    async def scan(self) -> list[DiscoveredDevice]:
        bleak = await _import_bleak()
        devices = await bleak.BleakScanner.discover(
            timeout=self.scan_timeout_s,
            service_uuids=[SERVICE_UUID],
        )
        results: list[DiscoveredDevice] = []
        for device in devices:
            name = device.name or ""
            metadata = getattr(device, "metadata", {}) or {}
            uuids = [uuid.lower() for uuid in metadata.get("uuids", [])]
            if name == self.name or SERVICE_UUID.lower() in uuids:
                results.append(
                    DiscoveredDevice(
                        name=name or "(unknown)",
                        address=device.address,
                        rssi=getattr(device, "rssi", None),
                    )
                )
        return results

    async def connect(self, name_or_address: str | None = None) -> None:
        bleak = await _import_bleak()
        target = name_or_address or self.name
        address = target
        if target == self.name or ":" not in target:
            devices = await self.scan()
            match = next(
                (device for device in devices if device.name == target or device.address == target),
                None,
            )
            if match is None:
                raise BleClientError(f"device not found: {target}")
            address = match.address

        client = bleak.BleakClient(address, disconnected_callback=self._handle_disconnect)
        self._client = client
        self._address = address
        try:
            await client.connect()
            if not client.is_connected:
                raise BleClientError(f"failed to connect: {address}")
        except BaseException:
            try:
                if client.is_connected:
                    await client.disconnect()
            finally:
                if self._client is client:
                    self._client = None
                    self._address = None
            raise

    async def disconnect(self) -> None:
        if self._client is not None:
            await self._client.disconnect()
        self._client = None
        self._address = None

    async def start_notifications(self, handler: NotificationHandler) -> None:
        client = self._require_client()
        self._notification_handler = handler
        await client.start_notify(SENSOR_DATA_UUID, self._on_notification)

    async def stop_notifications(self) -> None:
        client = self._require_client()
        await client.stop_notify(SENSOR_DATA_UUID)

    async def write_control(self, payload: bytes) -> None:
        client = self._require_client()
        await client.write_gatt_char(CONTROL_UUID, payload, response=True)

    async def write_config(self, payload: bytes) -> None:
        client = self._require_client()
        await client.write_gatt_char(CONFIG_UUID, payload, response=True)

    async def read_status(self) -> bytes:
        client = self._require_client()
        return bytes(await client.read_gatt_char(STATUS_UUID))

    async def read_config(self) -> bytes:
        client = self._require_client()
        return bytes(await client.read_gatt_char(CONFIG_UUID))

    async def read_capability(self) -> bytes:
        client = self._require_client()
        return bytes(await client.read_gatt_char(CAPABILITY_UUID))

    def set_disconnect_handler(self, handler: DisconnectHandler | None) -> None:
        self._disconnect_handler = handler

    def _require_client(self) -> Any:
        if self._client is None or not self._client.is_connected:
            raise BleClientError("not connected")
        return self._client

    def _on_notification(self, _sender: Any, data: bytearray) -> None:
        if self._notification_handler is not None:
            self._notification_handler(bytes(data))

    def _handle_disconnect(self, _client: Any) -> None:
        self._client = None
        self._address = None
        if self._disconnect_handler is not None:
            self._disconnect_handler()


async def _import_bleak() -> Any:
    try:
        import bleak
    except ImportError as exc:
        raise BleClientError("bleak is not installed; run `uv sync --extra dev` in pc_app") from exc
    await asyncio.sleep(0)
    return bleak

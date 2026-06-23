"""Binary protocol helpers for BLE Sensor Logger.

All payloads are little-endian and fixed length. This module stays independent
from bleak and UI code so the CUI and WebGUI can share it unchanged.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
import struct


PROTOCOL_VERSION = 3
CONFIG_VERSION = 4
MIN_INTERVAL_MS = 20
MAX_INTERVAL_MS = 10_000
STREAM_ID_DUMMY_ACCEL3 = 1
STREAM_ID_LSM6DSL_IMU6 = 10
STREAM_ID_LSM6DSL_ORIENTATION_MOTION = 13
STREAM_ID_LSM303AGR_MAG3 = 12
STREAM_ID_LPS22HB_PRESSURE = 20
STREAM_ID_HTS221_TEMP_HUMIDITY = 30
LSM6DSL_INTERVAL_MS = 38
LSM303AGR_MAG3_INTERVAL_MS = 100
LPS22HB_INTERVAL_MS = 1000
HTS221_INTERVAL_MS = 1000

SENSOR_DATA_HEADER_FORMAT = "<BBBBHIBB"
SENSOR_DUMMY_ACCEL3_SAMPLE_FORMAT = "<hhh"
SENSOR_IMU6_SAMPLE_FORMAT = "<hhhhhh"
SENSOR_ORIENTATION_MOTION_SAMPLE_FORMAT = "<hhhhhhhhhhhhhh"
SENSOR_MAG3_SAMPLE_FORMAT = "<hhh"
SENSOR_HTS221_SAMPLE_FORMAT = "<hh"
SENSOR_LPS22HB_SAMPLE_FORMAT = "<i"
CONTROL_FORMAT = "<BBH"
CONFIG_FORMAT = "<BBBBHH"
STATUS_FORMAT = "<BBHHHHHHH"
CAPABILITY_HEADER_FORMAT = "<BBHBBHHH"
CAPABILITY_STREAM_FORMAT = "<BBBBBBHHHHbB"

SENSOR_DATA_HEADER_SIZE = struct.calcsize(SENSOR_DATA_HEADER_FORMAT)
SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE = struct.calcsize(SENSOR_DUMMY_ACCEL3_SAMPLE_FORMAT)
SENSOR_IMU6_SAMPLE_SIZE = struct.calcsize(SENSOR_IMU6_SAMPLE_FORMAT)
SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE = struct.calcsize(SENSOR_ORIENTATION_MOTION_SAMPLE_FORMAT)
SENSOR_MAG3_SAMPLE_SIZE = struct.calcsize(SENSOR_MAG3_SAMPLE_FORMAT)
SENSOR_HTS221_SAMPLE_SIZE = struct.calcsize(SENSOR_HTS221_SAMPLE_FORMAT)
SENSOR_LPS22HB_SAMPLE_SIZE = struct.calcsize(SENSOR_LPS22HB_SAMPLE_FORMAT)
SENSOR_DATA_SIZE = SENSOR_DATA_HEADER_SIZE + SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE
CONTROL_SIZE = struct.calcsize(CONTROL_FORMAT)
CONFIG_SIZE = struct.calcsize(CONFIG_FORMAT)
STATUS_SIZE = struct.calcsize(STATUS_FORMAT)
CAPABILITY_HEADER_SIZE = struct.calcsize(CAPABILITY_HEADER_FORMAT)
CAPABILITY_STREAM_SIZE = struct.calcsize(CAPABILITY_STREAM_FORMAT)
CAPABILITY_SCHEMA_VERSION = 1


class ProtocolError(ValueError):
    """Raised when a payload is malformed or unsupported."""


class Command(IntEnum):
    START_MEASUREMENT = 0x01
    STOP_MEASUREMENT = 0x02
    REQUEST_STATUS = 0x03
    RESET_SEQUENCE = 0x04


class ConfigOp(IntEnum):
    SET_STREAM_INTERVAL = 0x01
    SET_COMPLEMENTARY_ALPHA = 0x02
    SET_MAHONY_KP = 0x03
    SET_MAHONY_KI = 0x04
    SET_IIR_CUTOFF_MILLIHZ = 0x05


class DeviceState(IntEnum):
    IDLE = 0
    MEASURING = 1
    ERROR = 2


class DeviceError(IntEnum):
    NONE = 0
    INVALID_VERSION = 1
    INVALID_LENGTH = 2
    INVALID_COMMAND = 3
    INVALID_CONFIG = 4
    NOT_CONNECTED = 5
    NOT_SUBSCRIBED = 6
    LSM6DSL_NO_DEVICETREE = 7
    LSM6DSL_NOT_READY = 8
    LSM6DSL_CONFIG_FAILED = 9
    HTS221_NO_DEVICETREE = 10
    HTS221_NOT_READY = 11
    LPS22HB_NO_DEVICETREE = 12
    LPS22HB_NOT_READY = 13
    LSM303AGR_MAGN_NO_DEVICETREE = 14
    LSM303AGR_MAGN_NOT_READY = 15


class MessageType(IntEnum):
    SENSOR_SAMPLE = 0x01


class CapabilityFlag(IntEnum):
    CUSTOM_GATT = 0x0001
    FIXED_BINARY = 0x0002


class CapabilityFeature(IntEnum):
    INTERVAL_CONFIG = 0x0001
    STATUS_READ = 0x0002
    SENSOR_NOTIFY = 0x0004


class StreamType(IntEnum):
    DUMMY_ACCEL3 = 1
    IMU6 = 2
    TEMP_HUMIDITY = 3
    PRESSURE = 4
    MAG3 = 5
    ORIENTATION_MOTION = 6


class StreamDataType(IntEnum):
    INT16 = 1
    INT32 = 2


class StreamUnit(IntEnum):
    MIXED = 0
    PA = 1
    UT = 2
    MG = 3


class PayloadFormat(IntEnum):
    DUMMY_ACCEL3_INT16_V1 = 1
    IMU6_INT16_V1 = 2
    HTS221_TEMP_HUMIDITY_INT16_V1 = 3
    LPS22HB_PRESSURE_INT32_V1 = 4
    MAG3_INT16_V1 = 5
    ORIENTATION_MOTION_INT16_V1 = 6
    ORIENTATION_MOTION_INT16_V2 = 7


class StreamFlag(IntEnum):
    ENABLED_BY_DEFAULT = 0x0001
    MIXED_UNITS = 0x0002


@dataclass(frozen=True)
class SensorDataPayload:
    version: int
    message_type: MessageType
    stream_id: int
    flags: int
    sequence: int
    timestamp_ms: int
    payload_format: PayloadFormat
    payload_len: int
    accel_x_mg: int
    accel_y_mg: int
    accel_z_mg: int
    gyro_x_mdps: int = 0
    gyro_y_mdps: int = 0
    gyro_z_mdps: int = 0
    humidity_centi_percent: int = 0
    temperature_centi_c: int = 0
    pressure_pa: int = 0
    mag_x_ut: int = 0
    mag_y_ut: int = 0
    mag_z_ut: int = 0
    pitch_naive_cdeg: int = 0
    roll_naive_cdeg: int = 0
    zenith_naive_cdeg: int = 0
    pitch_iir_cdeg: int = 0
    roll_iir_cdeg: int = 0
    zenith_iir_cdeg: int = 0
    pitch_complementary_cdeg: int = 0
    roll_complementary_cdeg: int = 0
    zenith_complementary_cdeg: int = 0
    pitch_mahony_cdeg: int = 0
    roll_mahony_cdeg: int = 0
    zenith_mahony_cdeg: int = 0
    yaw_mahony_cdeg: int = 0
    accel_norm_mg: int = 0

    @classmethod
    def unpack(cls, data: bytes | bytearray | memoryview) -> "SensorDataPayload":
        raw = bytes(data)
        if len(raw) < SENSOR_DATA_HEADER_SIZE:
            raise ProtocolError(
                f"sensor payload length must be at least {SENSOR_DATA_HEADER_SIZE}, got {len(raw)}"
            )
        (
            version,
            message_type_value,
            stream_id,
            flags,
            sequence,
            timestamp_ms,
            payload_format_value,
            payload_len,
        ) = struct.unpack(SENSOR_DATA_HEADER_FORMAT, raw[:SENSOR_DATA_HEADER_SIZE])
        try:
            message_type = MessageType(message_type_value)
            payload_format = PayloadFormat(payload_format_value)
        except ValueError as exc:
            raise ProtocolError("sensor payload contains unknown enum value") from exc

        expected_size = SENSOR_DATA_HEADER_SIZE + payload_len
        if len(raw) != expected_size:
            raise ProtocolError(f"sensor payload length must be {expected_size}, got {len(raw)}")

        payload_raw = raw[SENSOR_DATA_HEADER_SIZE:]
        if payload_format == PayloadFormat.DUMMY_ACCEL3_INT16_V1:
            if payload_len != SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE:
                raise ProtocolError(
                    "dummy accel3 payload_len must be "
                    f"{SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE}, got {payload_len}"
                )
            accel_x_mg, accel_y_mg, accel_z_mg = struct.unpack(
                SENSOR_DUMMY_ACCEL3_SAMPLE_FORMAT, payload_raw
            )
            payload = cls(
                version=version,
                message_type=message_type,
                stream_id=stream_id,
                flags=flags,
                sequence=sequence,
                timestamp_ms=timestamp_ms,
                payload_format=payload_format,
                payload_len=payload_len,
                accel_x_mg=accel_x_mg,
                accel_y_mg=accel_y_mg,
                accel_z_mg=accel_z_mg,
            )
        elif payload_format == PayloadFormat.IMU6_INT16_V1:
            if payload_len != SENSOR_IMU6_SAMPLE_SIZE:
                raise ProtocolError(
                    f"IMU6 payload_len must be {SENSOR_IMU6_SAMPLE_SIZE}, got {payload_len}"
                )
            (
                accel_x_mg,
                accel_y_mg,
                accel_z_mg,
                gyro_x_mdps,
                gyro_y_mdps,
                gyro_z_mdps,
            ) = struct.unpack(SENSOR_IMU6_SAMPLE_FORMAT, payload_raw)
            payload = cls(
                version=version,
                message_type=message_type,
                stream_id=stream_id,
                flags=flags,
                sequence=sequence,
                timestamp_ms=timestamp_ms,
                payload_format=payload_format,
                payload_len=payload_len,
                accel_x_mg=accel_x_mg,
                accel_y_mg=accel_y_mg,
                accel_z_mg=accel_z_mg,
                gyro_x_mdps=gyro_x_mdps,
                gyro_y_mdps=gyro_y_mdps,
                gyro_z_mdps=gyro_z_mdps,
            )
        elif payload_format == PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1:
            if payload_len != SENSOR_HTS221_SAMPLE_SIZE:
                raise ProtocolError(
                    f"HTS221 payload_len must be {SENSOR_HTS221_SAMPLE_SIZE}, got {payload_len}"
                )
            humidity_centi_percent, temperature_centi_c = struct.unpack(
                SENSOR_HTS221_SAMPLE_FORMAT, payload_raw
            )
            payload = cls(
                version=version,
                message_type=message_type,
                stream_id=stream_id,
                flags=flags,
                sequence=sequence,
                timestamp_ms=timestamp_ms,
                payload_format=payload_format,
                payload_len=payload_len,
                accel_x_mg=0,
                accel_y_mg=0,
                accel_z_mg=0,
                humidity_centi_percent=humidity_centi_percent,
                temperature_centi_c=temperature_centi_c,
            )
        elif payload_format == PayloadFormat.MAG3_INT16_V1:
            if payload_len != SENSOR_MAG3_SAMPLE_SIZE:
                raise ProtocolError(
                    f"MAG3 payload_len must be {SENSOR_MAG3_SAMPLE_SIZE}, got {payload_len}"
                )
            mag_x_ut, mag_y_ut, mag_z_ut = struct.unpack(SENSOR_MAG3_SAMPLE_FORMAT, payload_raw)
            payload = cls(
                version=version,
                message_type=message_type,
                stream_id=stream_id,
                flags=flags,
                sequence=sequence,
                timestamp_ms=timestamp_ms,
                payload_format=payload_format,
                payload_len=payload_len,
                accel_x_mg=0,
                accel_y_mg=0,
                accel_z_mg=0,
                mag_x_ut=mag_x_ut,
                mag_y_ut=mag_y_ut,
                mag_z_ut=mag_z_ut,
            )
        elif payload_format == PayloadFormat.ORIENTATION_MOTION_INT16_V2:
            if payload_len != SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE:
                raise ProtocolError(
                    "ORIENTATION_MOTION payload_len must be "
                    f"{SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE}, got {payload_len}"
                )
            (
                pitch_naive_cdeg,
                roll_naive_cdeg,
                zenith_naive_cdeg,
                pitch_iir_cdeg,
                roll_iir_cdeg,
                zenith_iir_cdeg,
                pitch_complementary_cdeg,
                roll_complementary_cdeg,
                zenith_complementary_cdeg,
                pitch_mahony_cdeg,
                roll_mahony_cdeg,
                zenith_mahony_cdeg,
                yaw_mahony_cdeg,
                accel_norm_mg,
            ) = struct.unpack(SENSOR_ORIENTATION_MOTION_SAMPLE_FORMAT, payload_raw)
            payload = cls(
                version=version,
                message_type=message_type,
                stream_id=stream_id,
                flags=flags,
                sequence=sequence,
                timestamp_ms=timestamp_ms,
                payload_format=payload_format,
                payload_len=payload_len,
                accel_x_mg=0,
                accel_y_mg=0,
                accel_z_mg=0,
                pitch_naive_cdeg=pitch_naive_cdeg,
                roll_naive_cdeg=roll_naive_cdeg,
                zenith_naive_cdeg=zenith_naive_cdeg,
                pitch_iir_cdeg=pitch_iir_cdeg,
                roll_iir_cdeg=roll_iir_cdeg,
                zenith_iir_cdeg=zenith_iir_cdeg,
                pitch_complementary_cdeg=pitch_complementary_cdeg,
                roll_complementary_cdeg=roll_complementary_cdeg,
                zenith_complementary_cdeg=zenith_complementary_cdeg,
                pitch_mahony_cdeg=pitch_mahony_cdeg,
                roll_mahony_cdeg=roll_mahony_cdeg,
                zenith_mahony_cdeg=zenith_mahony_cdeg,
                yaw_mahony_cdeg=yaw_mahony_cdeg,
                accel_norm_mg=accel_norm_mg,
            )
        elif payload_format == PayloadFormat.LPS22HB_PRESSURE_INT32_V1:
            if payload_len != SENSOR_LPS22HB_SAMPLE_SIZE:
                raise ProtocolError(
                    f"LPS22HB payload_len must be {SENSOR_LPS22HB_SAMPLE_SIZE}, got {payload_len}"
                )
            (pressure_pa,) = struct.unpack(SENSOR_LPS22HB_SAMPLE_FORMAT, payload_raw)
            payload = cls(
                version=version,
                message_type=message_type,
                stream_id=stream_id,
                flags=flags,
                sequence=sequence,
                timestamp_ms=timestamp_ms,
                payload_format=payload_format,
                payload_len=payload_len,
                accel_x_mg=0,
                accel_y_mg=0,
                accel_z_mg=0,
                pressure_pa=pressure_pa,
            )
        else:
            raise ProtocolError(f"unsupported payload format: {payload_format}")
        payload._validate_header()
        return payload

    def pack(self) -> bytes:
        self._validate_header()
        header = struct.pack(
            SENSOR_DATA_HEADER_FORMAT,
            self.version,
            int(self.message_type),
            self.stream_id,
            self.flags,
            self.sequence,
            self.timestamp_ms,
            int(self.payload_format),
            self.payload_len,
        )
        if self.payload_format == PayloadFormat.DUMMY_ACCEL3_INT16_V1:
            return header + struct.pack(
                SENSOR_DUMMY_ACCEL3_SAMPLE_FORMAT,
                self.accel_x_mg,
                self.accel_y_mg,
                self.accel_z_mg,
            )
        if self.payload_format == PayloadFormat.IMU6_INT16_V1:
            return header + struct.pack(
                SENSOR_IMU6_SAMPLE_FORMAT,
                self.accel_x_mg,
                self.accel_y_mg,
                self.accel_z_mg,
                self.gyro_x_mdps,
                self.gyro_y_mdps,
                self.gyro_z_mdps,
            )
        if self.payload_format == PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1:
            return header + struct.pack(
                SENSOR_HTS221_SAMPLE_FORMAT,
                self.humidity_centi_percent,
                self.temperature_centi_c,
            )
        if self.payload_format == PayloadFormat.MAG3_INT16_V1:
            return header + struct.pack(
                SENSOR_MAG3_SAMPLE_FORMAT,
                self.mag_x_ut,
                self.mag_y_ut,
                self.mag_z_ut,
            )
        if self.payload_format == PayloadFormat.ORIENTATION_MOTION_INT16_V2:
            return header + struct.pack(
                SENSOR_ORIENTATION_MOTION_SAMPLE_FORMAT,
                self.pitch_naive_cdeg,
                self.roll_naive_cdeg,
                self.zenith_naive_cdeg,
                self.pitch_iir_cdeg,
                self.roll_iir_cdeg,
                self.zenith_iir_cdeg,
                self.pitch_complementary_cdeg,
                self.roll_complementary_cdeg,
                self.zenith_complementary_cdeg,
                self.pitch_mahony_cdeg,
                self.roll_mahony_cdeg,
                self.zenith_mahony_cdeg,
                self.yaw_mahony_cdeg,
                self.accel_norm_mg,
            )
        return header + struct.pack(SENSOR_LPS22HB_SAMPLE_FORMAT, self.pressure_pa)

    def _validate_header(self) -> None:
        if self.version != PROTOCOL_VERSION:
            raise ProtocolError(f"unsupported protocol version: {self.version}")
        if self.message_type != MessageType.SENSOR_SAMPLE:
            raise ProtocolError(f"unsupported message type: {self.message_type}")
        if (
            self.stream_id == STREAM_ID_DUMMY_ACCEL3
            and self.payload_format == PayloadFormat.DUMMY_ACCEL3_INT16_V1
            and self.payload_len == SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE
        ):
            return
        if (
            self.stream_id == STREAM_ID_LSM6DSL_IMU6
            and self.payload_format == PayloadFormat.IMU6_INT16_V1
            and self.payload_len == SENSOR_IMU6_SAMPLE_SIZE
        ):
            return
        if (
            self.stream_id == STREAM_ID_HTS221_TEMP_HUMIDITY
            and self.payload_format == PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1
            and self.payload_len == SENSOR_HTS221_SAMPLE_SIZE
        ):
            return
        if (
            self.stream_id == STREAM_ID_LSM303AGR_MAG3
            and self.payload_format == PayloadFormat.MAG3_INT16_V1
            and self.payload_len == SENSOR_MAG3_SAMPLE_SIZE
        ):
            return
        if (
            self.stream_id == STREAM_ID_LSM6DSL_ORIENTATION_MOTION
            and self.payload_format == PayloadFormat.ORIENTATION_MOTION_INT16_V2
            and self.payload_len == SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE
        ):
            return
        if (
            self.stream_id == STREAM_ID_LPS22HB_PRESSURE
            and self.payload_format == PayloadFormat.LPS22HB_PRESSURE_INT32_V1
            and self.payload_len == SENSOR_LPS22HB_SAMPLE_SIZE
        ):
            return
        if self.stream_id not in (
            STREAM_ID_DUMMY_ACCEL3,
            STREAM_ID_LSM6DSL_IMU6,
            STREAM_ID_LSM6DSL_ORIENTATION_MOTION,
            STREAM_ID_LSM303AGR_MAG3,
            STREAM_ID_LPS22HB_PRESSURE,
            STREAM_ID_HTS221_TEMP_HUMIDITY,
        ):
            raise ProtocolError(f"unsupported stream id: {self.stream_id}")
        supported_formats = (
            PayloadFormat.DUMMY_ACCEL3_INT16_V1,
            PayloadFormat.IMU6_INT16_V1,
            PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1,
            PayloadFormat.LPS22HB_PRESSURE_INT32_V1,
            PayloadFormat.MAG3_INT16_V1,
            PayloadFormat.ORIENTATION_MOTION_INT16_V2,
        )
        if self.payload_format not in supported_formats:
            raise ProtocolError(f"unsupported payload format: {self.payload_format}")
        if self.payload_format == PayloadFormat.DUMMY_ACCEL3_INT16_V1:
            raise ProtocolError(
                "sensor payload_len must be "
                f"{SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE}, got {self.payload_len}"
            )
        if self.payload_format == PayloadFormat.IMU6_INT16_V1:
            raise ProtocolError(
                f"sensor payload_len must be {SENSOR_IMU6_SAMPLE_SIZE}, got {self.payload_len}"
            )
        if self.payload_format == PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1:
            raise ProtocolError(
                f"sensor payload_len must be {SENSOR_HTS221_SAMPLE_SIZE}, got {self.payload_len}"
            )
        if self.payload_format == PayloadFormat.MAG3_INT16_V1:
            raise ProtocolError(
                f"sensor payload_len must be {SENSOR_MAG3_SAMPLE_SIZE}, got {self.payload_len}"
            )
        if self.payload_format == PayloadFormat.ORIENTATION_MOTION_INT16_V2:
            raise ProtocolError(
                "sensor payload_len must be "
                f"{SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE}, got {self.payload_len}"
            )
        raise ProtocolError(
            f"sensor payload_len must be {SENSOR_LPS22HB_SAMPLE_SIZE}, got {self.payload_len}"
        )


@dataclass(frozen=True)
class ControlPayload:
    version: int
    command: Command
    value: int = 0

    @classmethod
    def unpack(cls, data: bytes | bytearray | memoryview) -> "ControlPayload":
        raw = bytes(data)
        if len(raw) != CONTROL_SIZE:
            raise ProtocolError(f"control payload length must be {CONTROL_SIZE}, got {len(raw)}")
        version, command_value, value = struct.unpack(CONTROL_FORMAT, raw)
        if version != PROTOCOL_VERSION:
            raise ProtocolError(f"unsupported protocol version: {version}")
        try:
            command = Command(command_value)
        except ValueError as exc:
            raise ProtocolError(f"unsupported command: {command_value}") from exc
        return cls(version=version, command=command, value=value)

    @classmethod
    def from_command(cls, command: Command, value: int = 0) -> "ControlPayload":
        return cls(version=PROTOCOL_VERSION, command=command, value=value)

    def pack(self) -> bytes:
        if self.version != PROTOCOL_VERSION:
            raise ProtocolError(f"unsupported protocol version: {self.version}")
        return struct.pack(CONTROL_FORMAT, self.version, int(self.command), self.value)


@dataclass(frozen=True)
class ConfigPayload:
    version: int
    op: ConfigOp
    stream_id: int
    flags: int
    sample_interval_ms: int
    reserved: int = 0

    @classmethod
    def default(cls) -> "ConfigPayload":
        return cls(
            version=CONFIG_VERSION,
            op=ConfigOp.SET_STREAM_INTERVAL,
            stream_id=STREAM_ID_DUMMY_ACCEL3,
            flags=0,
            sample_interval_ms=100,
            reserved=0,
        )

    @classmethod
    def unpack(cls, data: bytes | bytearray | memoryview) -> "ConfigPayload":
        raw = bytes(data)
        if len(raw) != CONFIG_SIZE:
            raise ProtocolError(f"config payload length must be {CONFIG_SIZE}, got {len(raw)}")
        version, op_value, stream_id, flags, sample_interval_ms, reserved = struct.unpack(
            CONFIG_FORMAT, raw
        )
        try:
            op = ConfigOp(op_value)
        except ValueError as exc:
            raise ProtocolError(f"unsupported config op: {op_value}") from exc
        payload = cls(
            version=version,
            op=op,
            stream_id=stream_id,
            flags=flags,
            sample_interval_ms=sample_interval_ms,
            reserved=reserved,
        )
        payload.validate()
        return payload

    def pack(self) -> bytes:
        self.validate()
        return struct.pack(
            CONFIG_FORMAT,
            self.version,
            int(self.op),
            self.stream_id,
            self.flags,
            self.sample_interval_ms,
            self.reserved,
        )

    def validate(self) -> None:
        if self.version != CONFIG_VERSION:
            raise ProtocolError(f"unsupported protocol version: {self.version}")
        if self.flags != 0:
            raise ProtocolError("flags must be 0")
        if self.reserved != 0:
            raise ProtocolError("reserved must be 0")
        if self.op == ConfigOp.SET_STREAM_INTERVAL:
            if self.stream_id != STREAM_ID_DUMMY_ACCEL3:
                raise ProtocolError(f"unsupported stream id for interval config: {self.stream_id}")
            if (
                self.sample_interval_ms < MIN_INTERVAL_MS
                or self.sample_interval_ms > MAX_INTERVAL_MS
            ):
                raise ProtocolError(
                    f"sample_interval_ms must be {MIN_INTERVAL_MS}-{MAX_INTERVAL_MS}, "
                    f"got {self.sample_interval_ms}"
                )
            return
        if self.stream_id != STREAM_ID_LSM6DSL_ORIENTATION_MOTION:
            raise ProtocolError(f"unsupported stream id for filter config: {self.stream_id}")
        if self.op == ConfigOp.SET_COMPLEMENTARY_ALPHA:
            if self.sample_interval_ms > 1000:
                raise ProtocolError("complementary alpha must be 0.000-1.000")
            return
        if self.op in (ConfigOp.SET_MAHONY_KP, ConfigOp.SET_MAHONY_KI):
            if self.sample_interval_ms > 10_000:
                raise ProtocolError("Mahony gain must be 0.000-10.000")
            return
        if self.op == ConfigOp.SET_IIR_CUTOFF_MILLIHZ:
            if self.sample_interval_ms < 1 or self.sample_interval_ms > 13_000:
                raise ProtocolError("IIR cutoff must be 0.001-13.000 Hz")
            return
        raise ProtocolError(f"unsupported config op: {self.op}")


@dataclass(frozen=True)
class StatusPayload:
    version: int
    state: DeviceState
    sample_interval_ms: int
    last_error: DeviceError
    connection_count: int
    lsm6dsl_error: DeviceError = DeviceError.NONE
    hts221_error: DeviceError = DeviceError.NONE
    lps22hb_error: DeviceError = DeviceError.NONE
    lsm303agr_magn_error: DeviceError = DeviceError.NONE

    @classmethod
    def unpack(cls, data: bytes | bytearray | memoryview) -> "StatusPayload":
        raw = bytes(data)
        if len(raw) != STATUS_SIZE:
            raise ProtocolError(f"status payload length must be {STATUS_SIZE}, got {len(raw)}")
        (
            version,
            state_value,
            sample_interval_ms,
            error_value,
            connection_count,
            lsm6dsl_error_value,
            hts221_error_value,
            lps22hb_error_value,
            lsm303agr_magn_error_value,
        ) = struct.unpack(STATUS_FORMAT, raw)
        if version != PROTOCOL_VERSION:
            raise ProtocolError(f"unsupported protocol version: {version}")
        try:
            state = DeviceState(state_value)
            last_error = DeviceError(error_value)
            lsm6dsl_error = DeviceError(lsm6dsl_error_value)
            hts221_error = DeviceError(hts221_error_value)
            lps22hb_error = DeviceError(lps22hb_error_value)
            lsm303agr_magn_error = DeviceError(lsm303agr_magn_error_value)
        except ValueError as exc:
            raise ProtocolError("status payload contains unknown enum value") from exc
        return cls(
            version=version,
            state=state,
            sample_interval_ms=sample_interval_ms,
            last_error=last_error,
            connection_count=connection_count,
            lsm6dsl_error=lsm6dsl_error,
            hts221_error=hts221_error,
            lps22hb_error=lps22hb_error,
            lsm303agr_magn_error=lsm303agr_magn_error,
        )

    def pack(self) -> bytes:
        if self.version != PROTOCOL_VERSION:
            raise ProtocolError(f"unsupported protocol version: {self.version}")
        return struct.pack(
            STATUS_FORMAT,
            self.version,
            int(self.state),
            self.sample_interval_ms,
            int(self.last_error),
            self.connection_count,
            int(self.lsm6dsl_error),
            int(self.hts221_error),
            int(self.lps22hb_error),
            int(self.lsm303agr_magn_error),
        )


@dataclass(frozen=True)
class CapabilityStream:
    stream_id: int
    stream_type: StreamType
    channel_count: int
    data_type: StreamDataType
    unit: StreamUnit
    payload_format: PayloadFormat
    stream_flags: int
    default_interval_ms: int
    min_interval_ms: int
    max_interval_ms: int
    scale_exponent: int
    reserved: int = 0

    @classmethod
    def unpack(cls, data: bytes | bytearray | memoryview) -> "CapabilityStream":
        raw = bytes(data)
        if len(raw) != CAPABILITY_STREAM_SIZE:
            raise ProtocolError(
                f"capability stream length must be {CAPABILITY_STREAM_SIZE}, got {len(raw)}"
            )
        (
            stream_id,
            stream_type,
            channel_count,
            data_type,
            unit,
            payload_format,
            stream_flags,
            default_interval_ms,
            min_interval_ms,
            max_interval_ms,
            scale_exponent,
            reserved,
        ) = struct.unpack(CAPABILITY_STREAM_FORMAT, raw)
        try:
            return cls(
                stream_id=stream_id,
                stream_type=StreamType(stream_type),
                channel_count=channel_count,
                data_type=StreamDataType(data_type),
                unit=StreamUnit(unit),
                payload_format=PayloadFormat(payload_format),
                stream_flags=stream_flags,
                default_interval_ms=default_interval_ms,
                min_interval_ms=min_interval_ms,
                max_interval_ms=max_interval_ms,
                scale_exponent=scale_exponent,
                reserved=reserved,
            )
        except ValueError as exc:
            raise ProtocolError("capability stream contains unknown enum value") from exc

    def pack(self) -> bytes:
        return struct.pack(
            CAPABILITY_STREAM_FORMAT,
            self.stream_id,
            int(self.stream_type),
            self.channel_count,
            int(self.data_type),
            int(self.unit),
            int(self.payload_format),
            self.stream_flags,
            self.default_interval_ms,
            self.min_interval_ms,
            self.max_interval_ms,
            self.scale_exponent,
            self.reserved,
        )


@dataclass(frozen=True)
class CapabilityPayload:
    version: int
    schema_version: int
    capability_flags: int
    supported_commands: int
    supported_features: int
    preferred_mtu: int
    streams: tuple[CapabilityStream, ...]
    reserved: int = 0

    @classmethod
    def default(cls) -> "CapabilityPayload":
        return cls(
            version=PROTOCOL_VERSION,
            schema_version=CAPABILITY_SCHEMA_VERSION,
            capability_flags=int(CapabilityFlag.CUSTOM_GATT) | int(CapabilityFlag.FIXED_BINARY),
            supported_commands=sum(1 << int(command) for command in Command),
            supported_features=int(CapabilityFeature.INTERVAL_CONFIG)
            | int(CapabilityFeature.STATUS_READ)
            | int(CapabilityFeature.SENSOR_NOTIFY),
            preferred_mtu=SENSOR_DATA_HEADER_SIZE + SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE,
            streams=(
                CapabilityStream(
                    stream_id=1,
                    stream_type=StreamType.DUMMY_ACCEL3,
                    channel_count=3,
                    data_type=StreamDataType.INT16,
                    unit=StreamUnit.MG,
                    payload_format=PayloadFormat.DUMMY_ACCEL3_INT16_V1,
                    stream_flags=int(StreamFlag.ENABLED_BY_DEFAULT),
                    default_interval_ms=100,
                    min_interval_ms=MIN_INTERVAL_MS,
                    max_interval_ms=MAX_INTERVAL_MS,
                    scale_exponent=0,
                    reserved=0,
                ),
                CapabilityStream(
                    stream_id=STREAM_ID_LSM6DSL_IMU6,
                    stream_type=StreamType.IMU6,
                    channel_count=6,
                    data_type=StreamDataType.INT16,
                    unit=StreamUnit.MIXED,
                    payload_format=PayloadFormat.IMU6_INT16_V1,
                    stream_flags=int(StreamFlag.ENABLED_BY_DEFAULT) | int(StreamFlag.MIXED_UNITS),
                    default_interval_ms=LSM6DSL_INTERVAL_MS,
                    min_interval_ms=LSM6DSL_INTERVAL_MS,
                    max_interval_ms=LSM6DSL_INTERVAL_MS,
                    scale_exponent=0,
                    reserved=0,
                ),
                CapabilityStream(
                    stream_id=STREAM_ID_LSM6DSL_ORIENTATION_MOTION,
                    stream_type=StreamType.ORIENTATION_MOTION,
                    channel_count=14,
                    data_type=StreamDataType.INT16,
                    unit=StreamUnit.MIXED,
                    payload_format=PayloadFormat.ORIENTATION_MOTION_INT16_V2,
                    stream_flags=int(StreamFlag.ENABLED_BY_DEFAULT) | int(StreamFlag.MIXED_UNITS),
                    default_interval_ms=LSM6DSL_INTERVAL_MS,
                    min_interval_ms=LSM6DSL_INTERVAL_MS,
                    max_interval_ms=LSM6DSL_INTERVAL_MS,
                    scale_exponent=-2,
                    reserved=0,
                ),
                CapabilityStream(
                    stream_id=STREAM_ID_HTS221_TEMP_HUMIDITY,
                    stream_type=StreamType.TEMP_HUMIDITY,
                    channel_count=2,
                    data_type=StreamDataType.INT16,
                    unit=StreamUnit.MIXED,
                    payload_format=PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1,
                    stream_flags=int(StreamFlag.ENABLED_BY_DEFAULT) | int(StreamFlag.MIXED_UNITS),
                    default_interval_ms=HTS221_INTERVAL_MS,
                    min_interval_ms=HTS221_INTERVAL_MS,
                    max_interval_ms=HTS221_INTERVAL_MS,
                    scale_exponent=-2,
                    reserved=0,
                ),
                CapabilityStream(
                    stream_id=STREAM_ID_LPS22HB_PRESSURE,
                    stream_type=StreamType.PRESSURE,
                    channel_count=1,
                    data_type=StreamDataType.INT32,
                    unit=StreamUnit.PA,
                    payload_format=PayloadFormat.LPS22HB_PRESSURE_INT32_V1,
                    stream_flags=int(StreamFlag.ENABLED_BY_DEFAULT),
                    default_interval_ms=LPS22HB_INTERVAL_MS,
                    min_interval_ms=LPS22HB_INTERVAL_MS,
                    max_interval_ms=LPS22HB_INTERVAL_MS,
                    scale_exponent=0,
                    reserved=0,
                ),
                CapabilityStream(
                    stream_id=STREAM_ID_LSM303AGR_MAG3,
                    stream_type=StreamType.MAG3,
                    channel_count=3,
                    data_type=StreamDataType.INT16,
                    unit=StreamUnit.UT,
                    payload_format=PayloadFormat.MAG3_INT16_V1,
                    stream_flags=int(StreamFlag.ENABLED_BY_DEFAULT),
                    default_interval_ms=LSM303AGR_MAG3_INTERVAL_MS,
                    min_interval_ms=LSM303AGR_MAG3_INTERVAL_MS,
                    max_interval_ms=LSM303AGR_MAG3_INTERVAL_MS,
                    scale_exponent=0,
                    reserved=0,
                ),
            ),
            reserved=0,
        )

    @classmethod
    def unpack(cls, data: bytes | bytearray | memoryview) -> "CapabilityPayload":
        raw = bytes(data)
        if len(raw) < CAPABILITY_HEADER_SIZE:
            raise ProtocolError(
                f"capability payload length must be at least {CAPABILITY_HEADER_SIZE}, got {len(raw)}"
            )
        (
            version,
            schema_version,
            capability_flags,
            stream_count,
            reserved,
            supported_commands,
            supported_features,
            preferred_mtu,
        ) = struct.unpack(CAPABILITY_HEADER_FORMAT, raw[:CAPABILITY_HEADER_SIZE])
        if version != PROTOCOL_VERSION:
            raise ProtocolError(f"unsupported protocol version: {version}")
        if schema_version != CAPABILITY_SCHEMA_VERSION:
            raise ProtocolError(f"unsupported capability schema version: {schema_version}")

        expected_size = CAPABILITY_HEADER_SIZE + stream_count * CAPABILITY_STREAM_SIZE
        if len(raw) != expected_size:
            raise ProtocolError(f"capability payload length must be {expected_size}, got {len(raw)}")

        streams = []
        offset = CAPABILITY_HEADER_SIZE
        for _ in range(stream_count):
            end = offset + CAPABILITY_STREAM_SIZE
            streams.append(CapabilityStream.unpack(raw[offset:end]))
            offset = end

        return cls(
            version=version,
            schema_version=schema_version,
            capability_flags=capability_flags,
            supported_commands=supported_commands,
            supported_features=supported_features,
            preferred_mtu=preferred_mtu,
            streams=tuple(streams),
            reserved=reserved,
        )

    def pack(self) -> bytes:
        if self.version != PROTOCOL_VERSION:
            raise ProtocolError(f"unsupported protocol version: {self.version}")
        if self.schema_version != CAPABILITY_SCHEMA_VERSION:
            raise ProtocolError(f"unsupported capability schema version: {self.schema_version}")
        header = struct.pack(
            CAPABILITY_HEADER_FORMAT,
            self.version,
            self.schema_version,
            self.capability_flags,
            len(self.streams),
            self.reserved,
            self.supported_commands,
            self.supported_features,
            self.preferred_mtu,
        )
        return header + b"".join(stream.pack() for stream in self.streams)

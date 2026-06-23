import pytest

from ble_sensor_logger.protocol import (
    CONFIG_SIZE,
    CONTROL_SIZE,
    CAPABILITY_HEADER_SIZE,
    CAPABILITY_STREAM_SIZE,
    SENSOR_DATA_HEADER_SIZE,
    SENSOR_HTS221_SAMPLE_SIZE,
    SENSOR_IMU6_SAMPLE_SIZE,
    SENSOR_LPS22HB_SAMPLE_SIZE,
    SENSOR_MAG3_SAMPLE_SIZE,
    SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE,
    SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE,
    SENSOR_DATA_SIZE,
    STATUS_SIZE,
    Command,
    CapabilityPayload,
    ConfigOp,
    ConfigPayload,
    ControlPayload,
    DeviceError,
    DeviceState,
    MessageType,
    PayloadFormat,
    ProtocolError,
    SensorDataPayload,
    StatusPayload,
)


def test_payload_sizes_match_spec():
    assert SENSOR_DATA_HEADER_SIZE == 12
    assert SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE == 6
    assert SENSOR_IMU6_SAMPLE_SIZE == 12
    assert SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE == 14
    assert SENSOR_MAG3_SAMPLE_SIZE == 6
    assert SENSOR_HTS221_SAMPLE_SIZE == 4
    assert SENSOR_LPS22HB_SAMPLE_SIZE == 4
    assert SENSOR_DATA_SIZE == 24
    assert CONTROL_SIZE == 4
    assert CONFIG_SIZE == 8
    assert STATUS_SIZE == 16
    assert CAPABILITY_HEADER_SIZE == 12
    assert CAPABILITY_STREAM_SIZE == 16


def test_sensor_data_round_trip():
    payload = SensorDataPayload(
        version=3,
        message_type=MessageType.SENSOR_SAMPLE,
        stream_id=1,
        flags=0,
        sequence=42,
        timestamp_ms=123456,
        payload_format=PayloadFormat.DUMMY_ACCEL3_INT16_V1,
        payload_len=SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE,
        accel_x_mg=-100,
        accel_y_mg=0,
        accel_z_mg=980,
    )
    assert SensorDataPayload.unpack(payload.pack()) == payload


def test_imu6_sensor_data_round_trip():
    payload = SensorDataPayload(
        version=3,
        message_type=MessageType.SENSOR_SAMPLE,
        stream_id=10,
        flags=0,
        sequence=7,
        timestamp_ms=456,
        payload_format=PayloadFormat.IMU6_INT16_V1,
        payload_len=SENSOR_IMU6_SAMPLE_SIZE,
        accel_x_mg=12,
        accel_y_mg=-34,
        accel_z_mg=1001,
        gyro_x_mdps=100,
        gyro_y_mdps=-200,
        gyro_z_mdps=300,
    )
    packed = payload.pack()

    assert len(packed) == SENSOR_DATA_HEADER_SIZE + SENSOR_IMU6_SAMPLE_SIZE
    assert SensorDataPayload.unpack(packed) == payload


def test_hts221_sensor_data_round_trip():
    payload = SensorDataPayload(
        version=3,
        message_type=MessageType.SENSOR_SAMPLE,
        stream_id=30,
        flags=0,
        sequence=3,
        timestamp_ms=789,
        payload_format=PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1,
        payload_len=SENSOR_HTS221_SAMPLE_SIZE,
        accel_x_mg=0,
        accel_y_mg=0,
        accel_z_mg=0,
        humidity_centi_percent=4567,
        temperature_centi_c=2345,
    )
    packed = payload.pack()

    assert len(packed) == SENSOR_DATA_HEADER_SIZE + SENSOR_HTS221_SAMPLE_SIZE
    assert SensorDataPayload.unpack(packed) == payload


def test_lps22hb_sensor_data_round_trip():
    payload = SensorDataPayload(
        version=3,
        message_type=MessageType.SENSOR_SAMPLE,
        stream_id=20,
        flags=0,
        sequence=4,
        timestamp_ms=790,
        payload_format=PayloadFormat.LPS22HB_PRESSURE_INT32_V1,
        payload_len=SENSOR_LPS22HB_SAMPLE_SIZE,
        accel_x_mg=0,
        accel_y_mg=0,
        accel_z_mg=0,
        pressure_pa=101325,
    )
    packed = payload.pack()

    assert len(packed) == SENSOR_DATA_HEADER_SIZE + SENSOR_LPS22HB_SAMPLE_SIZE
    assert SensorDataPayload.unpack(packed) == payload


def test_mag3_sensor_data_round_trip():
    payload = SensorDataPayload(
        version=3,
        message_type=MessageType.SENSOR_SAMPLE,
        stream_id=12,
        flags=0,
        sequence=5,
        timestamp_ms=791,
        payload_format=PayloadFormat.MAG3_INT16_V1,
        payload_len=SENSOR_MAG3_SAMPLE_SIZE,
        accel_x_mg=0,
        accel_y_mg=0,
        accel_z_mg=0,
        mag_x_ut=31,
        mag_y_ut=-4,
        mag_z_ut=42,
    )
    packed = payload.pack()

    assert len(packed) == SENSOR_DATA_HEADER_SIZE + SENSOR_MAG3_SAMPLE_SIZE
    assert SensorDataPayload.unpack(packed) == payload


def test_orientation_motion_sensor_data_round_trip():
    payload = SensorDataPayload(
        version=3,
        message_type=MessageType.SENSOR_SAMPLE,
        stream_id=13,
        flags=0,
        sequence=6,
        timestamp_ms=792,
        payload_format=PayloadFormat.ORIENTATION_MOTION_INT16_V1,
        payload_len=SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE,
        accel_x_mg=0,
        accel_y_mg=0,
        accel_z_mg=0,
        pitch_naive_cdeg=-1234,
        roll_naive_cdeg=567,
        zenith_naive_cdeg=9012,
        pitch_filtered_cdeg=-1200,
        roll_filtered_cdeg=600,
        zenith_filtered_cdeg=9000,
        accel_norm_mg=1001,
    )
    packed = payload.pack()

    assert len(packed) == SENSOR_DATA_HEADER_SIZE + SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE
    assert SensorDataPayload.unpack(packed) == payload


def test_control_round_trip():
    payload = ControlPayload.from_command(Command.START_MEASUREMENT)
    assert payload.pack() == b"\x03\x01\x00\x00"
    assert ControlPayload.unpack(payload.pack()) == payload


def test_config_rejects_interval_out_of_range():
    payload = ConfigPayload(
        version=4,
        op=ConfigOp.SET_STREAM_INTERVAL,
        stream_id=1,
        flags=0,
        sample_interval_ms=10,
        reserved=0,
    )
    with pytest.raises(ProtocolError):
        payload.pack()


def test_config_v4_stream_interval_round_trip():
    payload = ConfigPayload(
        version=4,
        op=ConfigOp.SET_STREAM_INTERVAL,
        stream_id=1,
        flags=0,
        sample_interval_ms=250,
        reserved=0,
    )

    assert payload.pack() == b"\x04\x01\x01\x00\xfa\x00\x00\x00"
    assert ConfigPayload.unpack(payload.pack()) == payload


def test_status_round_trip():
    payload = StatusPayload(
        version=3,
        state=DeviceState.MEASURING,
        sample_interval_ms=100,
        last_error=DeviceError.NONE,
        connection_count=2,
    )
    assert StatusPayload.unpack(payload.pack()) == payload


def test_status_round_trip_with_lsm6dsl_diagnostic():
    payload = StatusPayload(
        version=3,
        state=DeviceState.MEASURING,
        sample_interval_ms=100,
        last_error=DeviceError.LSM6DSL_NOT_READY,
        connection_count=2,
        lsm6dsl_error=DeviceError.LSM6DSL_NOT_READY,
    )
    assert StatusPayload.unpack(payload.pack()) == payload


def test_status_round_trip_with_hts221_diagnostic():
    payload = StatusPayload(
        version=3,
        state=DeviceState.MEASURING,
        sample_interval_ms=100,
        last_error=DeviceError.HTS221_NOT_READY,
        connection_count=2,
        hts221_error=DeviceError.HTS221_NOT_READY,
    )
    assert StatusPayload.unpack(payload.pack()) == payload


def test_status_round_trip_with_lps22hb_diagnostic():
    payload = StatusPayload(
        version=3,
        state=DeviceState.MEASURING,
        sample_interval_ms=100,
        last_error=DeviceError.LPS22HB_NOT_READY,
        connection_count=2,
        lps22hb_error=DeviceError.LPS22HB_NOT_READY,
    )
    assert StatusPayload.unpack(payload.pack()) == payload


def test_status_round_trip_with_lsm303agr_magn_diagnostic():
    payload = StatusPayload(
        version=3,
        state=DeviceState.MEASURING,
        sample_interval_ms=100,
        last_error=DeviceError.LSM303AGR_MAGN_NOT_READY,
        connection_count=2,
        lsm303agr_magn_error=DeviceError.LSM303AGR_MAGN_NOT_READY,
    )
    assert StatusPayload.unpack(payload.pack()) == payload


def test_status_round_trip_with_multiple_optional_sensor_diagnostics():
    payload = StatusPayload(
        version=3,
        state=DeviceState.IDLE,
        sample_interval_ms=100,
        last_error=DeviceError.LSM6DSL_NOT_READY,
        connection_count=1,
        lsm6dsl_error=DeviceError.LSM6DSL_NOT_READY,
        hts221_error=DeviceError.HTS221_NOT_READY,
        lps22hb_error=DeviceError.LPS22HB_NOT_READY,
        lsm303agr_magn_error=DeviceError.LSM303AGR_MAGN_NOT_READY,
    )
    assert StatusPayload.unpack(payload.pack()) == payload


def test_capability_round_trip():
    payload = CapabilityPayload.default()

    parsed = CapabilityPayload.unpack(payload.pack())

    assert parsed == payload
    assert parsed.streams[0].stream_id == 1
    assert parsed.streams[0].payload_format.name == "DUMMY_ACCEL3_INT16_V1"
    assert parsed.streams[1].stream_id == 10
    assert parsed.streams[1].payload_format.name == "IMU6_INT16_V1"
    assert parsed.streams[2].stream_id == 30
    assert parsed.streams[2].payload_format.name == "HTS221_TEMP_HUMIDITY_INT16_V1"
    assert parsed.streams[3].stream_id == 20
    assert parsed.streams[3].payload_format.name == "LPS22HB_PRESSURE_INT32_V1"
    assert parsed.streams[4].stream_id == 12
    assert parsed.streams[4].payload_format.name == "MAG3_INT16_V1"
    assert parsed.streams[5].stream_id == 13
    assert parsed.streams[5].stream_type.name == "ORIENTATION_MOTION"
    assert parsed.streams[5].payload_format.name == "ORIENTATION_MOTION_INT16_V1"
    assert parsed.streams[5].channel_count == 7
    assert parsed.preferred_mtu == SENSOR_DATA_HEADER_SIZE + SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE
    assert parsed.supported_commands & (1 << int(Command.START_MEASUREMENT))

import asyncio

from ble_sensor_logger.app_core import SensorLoggerApp
from ble_sensor_logger.protocol import (
    SENSOR_DATA_HEADER_SIZE,
    SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE,
    SENSOR_HTS221_SAMPLE_SIZE,
    SENSOR_IMU6_SAMPLE_SIZE,
    SENSOR_LPS22HB_SAMPLE_SIZE,
    SENSOR_MAG3_SAMPLE_SIZE,
    SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE,
    CapabilityPayload,
    ConfigOp,
    ConfigPayload,
    MessageType,
    PayloadFormat,
    SensorDataPayload,
)


class FakeClient:
    def __init__(self):
        self.disconnect_handler = None
        self.config_writes = []

    def set_disconnect_handler(self, handler):
        self.disconnect_handler = handler

    async def write_config(self, payload):
        self.config_writes.append(payload)


def test_sequence_gap_tracking():
    app = SensorLoggerApp(client=FakeClient())
    seen = []
    app._sample_handler = seen.append

    for sequence in (1, 2, 5):
        sample = SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=1,
            flags=0,
            sequence=sequence,
            timestamp_ms=sequence * 100,
            payload_format=PayloadFormat.DUMMY_ACCEL3_INT16_V1,
            payload_len=SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=1000,
        )
        app._handle_sensor_data(sample.pack())

    assert [sample.sequence for sample in seen] == [1, 2, 5]
    assert app.state.missed_samples == 2


def test_sequence_gap_tracking_with_batched_frame():
    app = SensorLoggerApp(client=FakeClient())
    seen = []
    app._sample_handler = seen.append

    sample1 = SensorDataPayload(
        version=4,
        message_type=MessageType.SENSOR_SAMPLE,
        stream_id=10,
        flags=0,
        sequence=10,
        timestamp_ms=1000,
        payload_format=PayloadFormat.IMU6_INT16_V1,
        payload_len=SENSOR_IMU6_SAMPLE_SIZE,
        sample_count=1,
        accel_x_mg=0,
        accel_y_mg=0,
        accel_z_mg=1000,
        gyro_x_mdps=10,
        gyro_y_mdps=20,
        gyro_z_mdps=30,
    )
    sample2 = SensorDataPayload(
        version=4,
        message_type=MessageType.SENSOR_SAMPLE,
        stream_id=10,
        flags=0,
        sequence=11,
        timestamp_ms=1038,
        payload_format=PayloadFormat.IMU6_INT16_V1,
        payload_len=SENSOR_IMU6_SAMPLE_SIZE,
        sample_count=1,
        accel_x_mg=1,
        accel_y_mg=1,
        accel_z_mg=1001,
        gyro_x_mdps=11,
        gyro_y_mdps=21,
        gyro_z_mdps=31,
    )
    packed1 = sample1.pack()
    packed2 = sample2.pack()
    batched = (
        packed1[:11]
        + bytes([SENSOR_IMU6_SAMPLE_SIZE * 2, 2])
        + packed1[SENSOR_DATA_HEADER_SIZE:]
        + packed2[SENSOR_DATA_HEADER_SIZE:]
    )

    app._handle_sensor_data(batched)

    assert [sample.sequence for sample in seen] == [10, 11]
    assert app.state.missed_samples == 0


def test_sequence_gap_tracking_is_per_stream():
    app = SensorLoggerApp(client=FakeClient())
    seen = []
    app._sample_handler = seen.append

    samples = (
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=1,
            flags=0,
            sequence=1,
            timestamp_ms=100,
            payload_format=PayloadFormat.DUMMY_ACCEL3_INT16_V1,
            payload_len=SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=1000,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=10,
            flags=0,
            sequence=1,
            timestamp_ms=101,
            payload_format=PayloadFormat.IMU6_INT16_V1,
            payload_len=SENSOR_IMU6_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=1000,
            gyro_x_mdps=10,
            gyro_y_mdps=20,
            gyro_z_mdps=30,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=30,
            flags=0,
            sequence=1,
            timestamp_ms=102,
            payload_format=PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1,
            payload_len=SENSOR_HTS221_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=0,
            humidity_centi_percent=5000,
            temperature_centi_c=2500,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=20,
            flags=0,
            sequence=1,
            timestamp_ms=103,
            payload_format=PayloadFormat.LPS22HB_PRESSURE_INT32_V1,
            payload_len=SENSOR_LPS22HB_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=0,
            pressure_pa=101325,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=12,
            flags=0,
            sequence=1,
            timestamp_ms=104,
            payload_format=PayloadFormat.MAG3_INT16_V1,
            payload_len=SENSOR_MAG3_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=0,
            mag_x_ut=31,
            mag_y_ut=-4,
            mag_z_ut=42,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=13,
            flags=0,
            sequence=1,
            timestamp_ms=105,
            payload_format=PayloadFormat.ORIENTATION_MOTION_INT16_V2,
            payload_len=SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=0,
            pitch_naive_cdeg=-1234,
            roll_naive_cdeg=567,
            zenith_naive_cdeg=9012,
            pitch_iir_cdeg=-1220,
            roll_iir_cdeg=590,
            zenith_iir_cdeg=9005,
            pitch_complementary_cdeg=-1200,
            roll_complementary_cdeg=600,
            zenith_complementary_cdeg=9000,
            pitch_mahony_cdeg=-1190,
            roll_mahony_cdeg=610,
            zenith_mahony_cdeg=8990,
            yaw_mahony_cdeg=42,
            accel_norm_mg=1001,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=1,
            flags=0,
            sequence=2,
            timestamp_ms=200,
            payload_format=PayloadFormat.DUMMY_ACCEL3_INT16_V1,
            payload_len=SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=1000,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=10,
            flags=0,
            sequence=2,
            timestamp_ms=201,
            payload_format=PayloadFormat.IMU6_INT16_V1,
            payload_len=SENSOR_IMU6_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=1000,
            gyro_x_mdps=10,
            gyro_y_mdps=20,
            gyro_z_mdps=30,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=30,
            flags=0,
            sequence=2,
            timestamp_ms=202,
            payload_format=PayloadFormat.HTS221_TEMP_HUMIDITY_INT16_V1,
            payload_len=SENSOR_HTS221_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=0,
            humidity_centi_percent=5001,
            temperature_centi_c=2501,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=20,
            flags=0,
            sequence=2,
            timestamp_ms=203,
            payload_format=PayloadFormat.LPS22HB_PRESSURE_INT32_V1,
            payload_len=SENSOR_LPS22HB_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=0,
            pressure_pa=101326,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=12,
            flags=0,
            sequence=2,
            timestamp_ms=204,
            payload_format=PayloadFormat.MAG3_INT16_V1,
            payload_len=SENSOR_MAG3_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=0,
            mag_x_ut=32,
            mag_y_ut=-5,
            mag_z_ut=43,
        ),
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=13,
            flags=0,
            sequence=2,
            timestamp_ms=205,
            payload_format=PayloadFormat.ORIENTATION_MOTION_INT16_V2,
            payload_len=SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=0,
            pitch_naive_cdeg=-1233,
            roll_naive_cdeg=568,
            zenith_naive_cdeg=9011,
            pitch_iir_cdeg=-1219,
            roll_iir_cdeg=591,
            zenith_iir_cdeg=9004,
            pitch_complementary_cdeg=-1199,
            roll_complementary_cdeg=601,
            zenith_complementary_cdeg=8999,
            pitch_mahony_cdeg=-1189,
            roll_mahony_cdeg=611,
            zenith_mahony_cdeg=8989,
            yaw_mahony_cdeg=43,
            accel_norm_mg=1002,
        ),
    )

    for sample in samples:
        app._handle_sensor_data(sample.pack())

    assert [(sample.stream_id, sample.sequence) for sample in seen] == [
        (1, 1),
        (10, 1),
        (30, 1),
        (20, 1),
        (12, 1),
        (13, 1),
        (1, 2),
        (10, 2),
        (30, 2),
        (20, 2),
        (12, 2),
        (13, 2),
    ]
    assert app.state.missed_samples == 0


def test_read_capability_uses_client_payload():
    class CapabilityClient(FakeClient):
        async def read_capability(self):
            return CapabilityPayload.default().pack()

    async def run():
        app = SensorLoggerApp(client=CapabilityClient())

        capability = await app.read_capability()

        assert capability.schema_version == 1
        assert capability.streams[0].payload_format.name == "DUMMY_ACCEL3_INT16_V1"
        assert capability.streams[1].payload_format.name == "IMU6_INT16_V1"
        assert capability.streams[2].payload_format.name == "ORIENTATION_MOTION_INT16_V2"
        assert capability.streams[3].payload_format.name == "HTS221_TEMP_HUMIDITY_INT16_V1"
        assert capability.streams[4].payload_format.name == "LPS22HB_PRESSURE_INT32_V1"
        assert capability.streams[5].payload_format.name == "MAG3_INT16_V1"

    asyncio.run(run())


def test_set_stream_interval_writes_config_v4_payload():
    async def run():
        client = FakeClient()
        app = SensorLoggerApp(client=client)

        await app.set_stream_interval(1, 250)

        assert len(client.config_writes) == 1
        payload = ConfigPayload.unpack(client.config_writes[0])
        assert payload.version == 4
        assert payload.stream_id == 1
        assert payload.sample_interval_ms == 250

    asyncio.run(run())


def test_set_orientation_filter_params_writes_config_v4_payloads():
    async def run():
        client = FakeClient()
        app = SensorLoggerApp(client=client)

        await app.set_orientation_filter_params(0.975, 0.5, 0.01, 2.5)

        assert len(client.config_writes) == 4
        payloads = [ConfigPayload.unpack(payload) for payload in client.config_writes]
        assert [payload.op for payload in payloads] == [
            ConfigOp.SET_COMPLEMENTARY_ALPHA,
            ConfigOp.SET_MAHONY_KP,
            ConfigOp.SET_MAHONY_KI,
            ConfigOp.SET_IIR_CUTOFF_MILLIHZ,
        ]
        assert [payload.stream_id for payload in payloads] == [13, 13, 13, 13]
        assert [payload.sample_interval_ms for payload in payloads] == [975, 500, 10, 2500]

    asyncio.run(run())


class FakeClientWithControl(FakeClient):
    def __init__(self):
        super().__init__()
        self.control_writes = []

    async def write_control(self, payload):
        self.control_writes.append(payload)


def test_force_gyro_calib_writes_control_command():
    from ble_sensor_logger.protocol import Command, ControlPayload

    async def run():
        client = FakeClientWithControl()
        app = SensorLoggerApp(client=client)
        await app.force_gyro_calib()
        assert len(client.control_writes) == 1
        payload = ControlPayload.unpack(client.control_writes[0])
        assert payload.command == Command.FORCE_GYRO_CALIB

    asyncio.run(run())


def test_set_gyro_calib_params_writes_config_v4_payloads():
    async def run():
        client = FakeClient()
        app = SensorLoggerApp(client=client)
        await app.set_gyro_calib_params(threshold_mdps=80, alpha_permille=10, window_samples=52)

        assert len(client.config_writes) == 3
        payloads = [ConfigPayload.unpack(payload) for payload in client.config_writes]
        assert [payload.op for payload in payloads] == [
            ConfigOp.SET_GYRO_CALIB_THRESHOLD,
            ConfigOp.SET_GYRO_CALIB_ALPHA,
            ConfigOp.SET_GYRO_CALIB_WINDOW,
        ]
        assert [payload.stream_id for payload in payloads] == [10, 10, 10]
        assert [payload.sample_interval_ms for payload in payloads] == [80, 10, 52]

    asyncio.run(run())

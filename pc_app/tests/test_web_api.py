import asyncio
import json
from pathlib import Path
from types import SimpleNamespace

from ble_sensor_logger.web_api import WebBackend, create_web_app
from ble_sensor_logger.protocol import (
    SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE,
    SENSOR_HTS221_SAMPLE_SIZE,
    SENSOR_IMU6_SAMPLE_SIZE,
    SENSOR_MAG3_SAMPLE_SIZE,
    SENSOR_ORIENTATION_MOTION_SAMPLE_SIZE,
    CapabilityPayload,
    DeviceError,
    DeviceState,
    MessageType,
    PayloadFormat,
    SensorDataPayload,
    StatusPayload,
)


def test_web_app_exposes_expected_routes():
    app = create_web_app()
    routes = {(route.method, route.resource.canonical) for route in app.router.routes()}

    assert ("GET", "/api/scan") in routes
    assert ("POST", "/api/connect") in routes
    assert ("POST", "/api/connect/cancel") in routes
    assert ("POST", "/api/start") in routes
    assert ("POST", "/api/interval") in routes
    assert ("POST", "/api/orientation-filter") in routes
    assert ("GET", "/api/status") in routes
    assert ("GET", "/api/capability") in routes
    assert ("GET", "/ws") in routes
    assert ("GET", "/") in routes


def test_web_frontend_assets_exist():
    web_root = Path(__file__).resolve().parents[1] / "web_frontend"

    assert (web_root / "index.html").is_file()
    assert (web_root / "styles.css").is_file()
    assert (web_root / "app.js").is_file()
    assert (web_root / "vendor" / "three.module.min.js").is_file()


def test_web_frontend_signals_are_capability_driven():
    web_root = Path(__file__).resolve().parents[1] / "web_frontend"
    index_html = (web_root / "index.html").read_text()
    app_js = (web_root / "app.js").read_text()

    assert '<section id="metricsGrid" class="metrics-grid"' in index_html
    assert 'id="graphsGrid"' in index_html
    assert index_html.count('data-role="graph-canvas"') == 3
    assert index_html.count('data-role="metric-selects"') == 3
    assert index_html.count('class="graph-options"') == 3
    assert 'data-role="y-mode"' in index_html
    assert 'data-role="x-mode"' in index_html
    assert 'value="imu_accel_z_mg">LSM6DSL Accel Z' not in index_html
    assert "renderCapability(payload.capability, true)" in app_js
    assert "fieldDefinitions = flattenFields(capability)" in app_js
    assert 'id="streamSelect"' in index_html
    assert 'type="module" src="/static/app.js"' in index_html
    assert "configurableStreams = (capability.streams || []).filter" in app_js
    assert "stream_id: Number(elements.streamSelect.value)" in app_js
    assert "metricId(stream.stream_id, field.field)" in app_js
    assert "const maxGraphSignals = 3" in app_js
    assert "const graphConfigs = [" in app_js
    assert "renderGraphSignalControls()" in app_js
    assert "compactSignalLabel" in app_js
    assert "config.yMode === \"manual\"" in app_js
    assert "config.xMode === \"seconds\"" in app_js
    assert "Dummy Accel Z" in app_js
    assert "LSM6DSL Accel Z" in app_js
    assert "LSM303AGR Mag Z" in app_js
    assert 'id="lsm6dslError"' in index_html
    assert "optional_sensors?.lsm6dsl?.error" in app_js
    assert 'id="lsm303agrMagnError"' in index_html
    assert "optional_sensors?.lsm303agr_magn?.error" in app_js


def test_web_frontend_has_orientation_view():
    web_root = Path(__file__).resolve().parents[1] / "web_frontend"
    index_html = (web_root / "index.html").read_text()
    app_js = (web_root / "app.js").read_text()

    assert 'id="orientationCanvas"' in index_html
    assert 'id="orientationMode"' in index_html
    assert 'id="orientationNaivePitch"' in index_html
    assert 'id="orientationIirPitch"' in index_html
    assert 'id="orientationComplementaryPitch"' in index_html
    assert 'id="orientationMahonyPitch"' in index_html
    assert 'id="orientationMahonyYaw"' in index_html
    assert 'id="orientationToggleNaive"' in index_html
    assert 'id="orientationToggleIir"' in index_html
    assert 'id="iirCutoffInput"' in index_html
    assert 'id="applyFilterButton"' in index_html
    assert 'from "/static/vendor/three.module.min.js"' in app_js
    assert "new THREE.WebGLRenderer" in app_js
    assert "stream_id: 13" in app_js
    assert 'stream_type: "ORIENTATION_MOTION"' in app_js
    assert 'field: "pitch_complementary_cdeg"' in app_js
    assert 'field: "pitch_iir_cdeg"' in app_js
    assert 'field: "pitch_mahony_cdeg"' in app_js
    assert 'field: "yaw_mahony_cdeg"' in app_js
    assert 'field: "accel_norm_mg"' in app_js
    assert "orientationModeValues(source, mode)" in app_js
    assert 'commandAndRefresh("/api/orientation-filter", body)' in app_js
    assert "updateOrientationView(sample)" in app_js
    assert '<details class="live-raw-section" open>' in index_html


def test_web_frontend_csv_uses_stream_qualified_columns():
    web_root = Path(__file__).resolve().parents[1] / "web_frontend"
    app_js = (web_root / "app.js").read_text()

    assert "csvValueColumns = fieldDefinitions.map" in app_js
    assert "header: definition.metricId" in app_js
    assert "sampleMatchesStream(sample, column.streamId)" in app_js
    assert "sameSample(previous, sample)" in app_js
    assert "previous?.host_time_ms === sample.host_time_ms" not in app_js


class JsonRequest:
    def __init__(self, body):
        self.body = body

    async def json(self):
        return self.body


class SlowConnectApp:
    def __init__(self):
        self.state = SimpleNamespace(
            connected=False,
            monitoring=False,
            missed_samples=0,
        )
        self.disconnect_count = 0
        self.connect_started = asyncio.Event()
        self.release_connect = asyncio.Event()

    def set_disconnect_handler(self, _handler):
        pass

    async def connect(self, _target):
        self.connect_started.set()
        await self.release_connect.wait()
        self.state.connected = True

    async def disconnect(self):
        self.disconnect_count += 1
        self.state.connected = False
        self.state.monitoring = False

    async def start_monitoring(self, _handler):
        self.state.monitoring = True


def test_connect_can_be_cancelled_without_residual_connection():
    async def run():
        app = SlowConnectApp()
        backend = WebBackend(app)
        connect_request = JsonRequest({"target": "test-device"})
        connect_call = asyncio.create_task(backend.connect(connect_request))

        await app.connect_started.wait()
        assert backend.connecting is True
        assert backend.state_message()["connecting"] is True

        cancel_response = await backend.cancel_connect(None)
        connect_response = await connect_call

        assert cancel_response.status == 200
        assert connect_response.status == 200
        assert backend.connecting is False
        assert backend.connection_task is None
        assert app.state.connected is False
        assert app.state.monitoring is False
        assert app.disconnect_count >= 1

    asyncio.run(run())


def test_connect_failure_returns_json_error_and_cleans_up():
    class FailingConnectApp(SlowConnectApp):
        async def connect(self, _target):
            raise RuntimeError("connection failed")

    async def run():
        app = FailingConnectApp()
        backend = WebBackend(app)

        response = await backend.connect(JsonRequest({"target": "test-device"}))

        assert response.status == 502
        assert backend.connecting is False
        assert backend.connection_task is None
        assert app.state.connected is False
        assert app.disconnect_count >= 1

    asyncio.run(run())


def test_capability_response_uses_stream_metadata():
    class CapabilityApp(SlowConnectApp):
        def __init__(self):
            super().__init__()
            self.state.connected = True

        async def read_capability(self):
            return CapabilityPayload.default()

    async def run():
        backend = WebBackend(CapabilityApp())

        response = await backend.capability(None)

        assert response.status == 200
        body = response.text
        assert '"schema_version": 1' in body
        assert '"stream_type": "DUMMY_ACCEL3"' in body
        assert '"stream_type": "IMU6"' in body
        assert '"stream_type": "TEMP_HUMIDITY"' in body
        assert '"stream_type": "PRESSURE"' in body
        assert '"stream_type": "MAG3"' in body
        assert '"stream_type": "ORIENTATION_MOTION"' in body
        assert '"payload_format": "DUMMY_ACCEL3_INT16_V1"' in body
        assert '"payload_format": "IMU6_INT16_V1"' in body
        assert '"payload_format": "HTS221_TEMP_HUMIDITY_INT16_V1"' in body
        assert '"payload_format": "LPS22HB_PRESSURE_INT32_V1"' in body
        assert '"payload_format": "MAG3_INT16_V1"' in body
        assert '"payload_format": "ORIENTATION_MOTION_INT16_V2"' in body
        assert '"fields": [' in body
        assert '"label": "Dummy Accel Z"' in body
        assert '"label": "LSM6DSL Accel Z"' in body
        assert '"label": "LSM303AGR Mag Z"' in body
        assert '"label": "LSM6DSL Pitch Complementary"' in body
        assert '"label": "LSM6DSL Pitch IIR"' in body
        assert '"label": "LSM6DSL Pitch Mahony"' in body
        assert '"field": "pitch_iir_cdeg"' in body
        assert '"field": "yaw_mahony_cdeg"' in body
        assert '"field": "accel_norm_mg"' in body
        assert '"unit": "%RH"' in body
        assert '"unit": "degree"' in body
        assert '"scale": 0.01' in body

    asyncio.run(run())


def test_status_response_includes_optional_sensor_errors():
    class StatusApp(SlowConnectApp):
        def __init__(self):
            super().__init__()
            self.state.connected = True

        async def read_status(self):
            return StatusPayload(
                version=3,
                state=DeviceState.MEASURING,
                sample_interval_ms=100,
                last_error=DeviceError.LSM6DSL_NOT_READY,
                connection_count=1,
                lsm6dsl_error=DeviceError.LSM6DSL_NOT_READY,
                hts221_error=DeviceError.HTS221_NOT_READY,
                lps22hb_error=DeviceError.LPS22HB_NOT_READY,
                lsm303agr_magn_error=DeviceError.LSM303AGR_MAGN_NOT_READY,
            )

    async def run():
        backend = WebBackend(StatusApp())

        response = await backend.status(None)

        assert response.status == 200
        body = json.loads(response.text)
        assert body["status"]["last_error"] == "LSM6DSL_NOT_READY"
        assert body["status"]["optional_sensors"]["lsm6dsl"]["error"] == "LSM6DSL_NOT_READY"
        assert body["status"]["optional_sensors"]["hts221"]["error"] == "HTS221_NOT_READY"
        assert body["status"]["optional_sensors"]["lps22hb"]["error"] == "LPS22HB_NOT_READY"
        assert (
            body["status"]["optional_sensors"]["lsm303agr_magn"]["error"]
            == "LSM303AGR_MAGN_NOT_READY"
        )

    asyncio.run(run())


def test_set_interval_posts_stream_scoped_config():
    class IntervalApp(SlowConnectApp):
        def __init__(self):
            super().__init__()
            self.interval_calls = []

        async def set_stream_interval(self, stream_id, interval_ms):
            self.interval_calls.append((stream_id, interval_ms))

    async def run():
        app = IntervalApp()
        backend = WebBackend(app)

        response = await backend.set_interval(JsonRequest({"stream_id": 1, "interval_ms": 250}))

        assert response.status == 200
        assert app.interval_calls == [(1, 250)]
        body = json.loads(response.text)
        assert body == {"ok": True, "stream_id": 1, "interval_ms": 250}

    asyncio.run(run())


def test_set_orientation_filter_posts_filter_config():
    class FilterApp(SlowConnectApp):
        def __init__(self):
            super().__init__()
            self.filter_calls = []

        async def set_orientation_filter_params(
            self, complementary_alpha, mahony_kp, mahony_ki, iir_cutoff_hz
        ):
            self.filter_calls.append((complementary_alpha, mahony_kp, mahony_ki, iir_cutoff_hz))

    async def run():
        app = FilterApp()
        backend = WebBackend(app)

        response = await backend.set_orientation_filter(
            JsonRequest(
                {
                    "complementary_alpha": 0.97,
                    "mahony_kp": 0.6,
                    "mahony_ki": 0.02,
                    "iir_cutoff_hz": 2.5,
                }
            )
        )

        assert response.status == 200
        assert app.filter_calls == [(0.97, 0.6, 0.02, 2.5)]
        body = json.loads(response.text)
        assert body == {
            "ok": True,
            "complementary_alpha": 0.97,
            "mahony_kp": 0.6,
            "mahony_ki": 0.02,
            "iir_cutoff_hz": 2.5,
        }

    asyncio.run(run())


def test_web_sample_json_marks_fields_outside_payload_as_null():
    backend = WebBackend(SlowConnectApp())

    mixed = backend._sample_to_dict(
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=1,
            flags=0,
            sequence=1,
            timestamp_ms=100,
            payload_format=PayloadFormat.DUMMY_ACCEL3_INT16_V1,
            payload_len=SENSOR_DUMMY_ACCEL3_SAMPLE_SIZE,
            accel_x_mg=1,
            accel_y_mg=2,
            accel_z_mg=1000,
        )
    )
    assert mixed["accel_z_mg"] == 1000
    assert mixed["gyro_x_mdps"] is None
    assert mixed["humidity_centi_percent"] is None

    imu = backend._sample_to_dict(
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=10,
            flags=0,
            sequence=1,
            timestamp_ms=101,
            payload_format=PayloadFormat.IMU6_INT16_V1,
            payload_len=SENSOR_IMU6_SAMPLE_SIZE,
            accel_x_mg=4,
            accel_y_mg=5,
            accel_z_mg=1001,
            gyro_x_mdps=10,
            gyro_y_mdps=20,
            gyro_z_mdps=30,
        )
    )
    assert imu["accel_z_mg"] == 1001
    assert imu["gyro_z_mdps"] == 30

    hts221 = backend._sample_to_dict(
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
        )
    )
    assert hts221["humidity_centi_percent"] == 5000
    assert hts221["temperature_centi_c"] == 2500
    assert hts221["accel_z_mg"] is None

    mag3 = backend._sample_to_dict(
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=12,
            flags=0,
            sequence=1,
            timestamp_ms=103,
            payload_format=PayloadFormat.MAG3_INT16_V1,
            payload_len=SENSOR_MAG3_SAMPLE_SIZE,
            accel_x_mg=0,
            accel_y_mg=0,
            accel_z_mg=0,
            mag_x_ut=31,
            mag_y_ut=-4,
            mag_z_ut=42,
        )
    )
    assert mag3["mag_x_ut"] == 31
    assert mag3["mag_y_ut"] == -4
    assert mag3["mag_z_ut"] == 42
    assert mag3["accel_z_mg"] is None
    assert mag3["pressure_pa"] is None

    orientation = backend._sample_to_dict(
        SensorDataPayload(
            version=3,
            message_type=MessageType.SENSOR_SAMPLE,
            stream_id=13,
            flags=0,
            sequence=1,
            timestamp_ms=104,
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
        )
    )
    assert orientation["pitch_naive_cdeg"] == -1234
    assert orientation["pitch_iir_cdeg"] == -1220
    assert orientation["roll_complementary_cdeg"] == 600
    assert orientation["yaw_mahony_cdeg"] == 42
    assert orientation["accel_norm_mg"] == 1001
    assert orientation["accel_z_mg"] is None
    assert orientation["mag_z_ut"] is None
    assert orientation["pressure_pa"] is None

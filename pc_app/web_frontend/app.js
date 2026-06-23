import * as THREE from "/static/vendor/three.module.min.js";

const $ = (id) => document.getElementById(id);
const elements = {
  connectionLabel: $("connectionLabel"),
  scanButton: $("scanButton"),
  deviceSelect: $("deviceSelect"),
  connectButton: $("connectButton"),
  disconnectButton: $("disconnectButton"),
  startButton: $("startButton"),
  stopButton: $("stopButton"),
  resetButton: $("resetButton"),
  streamSelect: $("streamSelect"),
  intervalInput: $("intervalInput"),
  applyIntervalButton: $("applyIntervalButton"),
  complementaryAlphaInput: $("complementaryAlphaInput"),
  mahonyKpInput: $("mahonyKpInput"),
  mahonyKiInput: $("mahonyKiInput"),
  applyFilterButton: $("applyFilterButton"),
  refreshStatusButton: $("refreshStatusButton"),
  metricsGrid: $("metricsGrid"),
  clearButton: $("clearButton"),
  csvButton: $("csvButton"),
  csvState: $("csvState"),
  graphsGrid: $("graphsGrid"),
  orientationCanvas: $("orientationCanvas"),
  orientationState: $("orientationState"),
  orientationMode: $("orientationMode"),
  orientationToggleNaive: $("orientationToggleNaive"),
  orientationToggleComplementary: $("orientationToggleComplementary"),
  orientationToggleMahony: $("orientationToggleMahony"),
  orientationNaivePitch: $("orientationNaivePitch"),
  orientationNaiveRoll: $("orientationNaiveRoll"),
  orientationNaiveZenith: $("orientationNaiveZenith"),
  orientationNaiveAccel: $("orientationNaiveAccel"),
  orientationComplementaryPitch: $("orientationComplementaryPitch"),
  orientationComplementaryRoll: $("orientationComplementaryRoll"),
  orientationComplementaryZenith: $("orientationComplementaryZenith"),
  orientationComplementaryAccel: $("orientationComplementaryAccel"),
  orientationMahonyPitch: $("orientationMahonyPitch"),
  orientationMahonyRoll: $("orientationMahonyRoll"),
  orientationMahonyZenith: $("orientationMahonyZenith"),
  orientationMahonyAccel: $("orientationMahonyAccel"),
  orientationMahonyYaw: $("orientationMahonyYaw"),
  toast: $("toast"),
};

const history = [];
const maxHistoryPoints = 5000;
const maxGraphSignals = 3;
const graphColors = ["#176b5b", "#2f6fb5", "#a8482d"];
const orientationModes = {
  naive: {
    label: "Naive",
    color: 0x2f6fb5,
    toggle: "orientationToggleNaive",
    fields: {
      pitch: "pitch_naive_cdeg",
      roll: "roll_naive_cdeg",
      zenith: "zenith_naive_cdeg",
      accel: "accel_norm_mg",
    },
    readout: {
      pitch: "orientationNaivePitch",
      roll: "orientationNaiveRoll",
      zenith: "orientationNaiveZenith",
      accel: "orientationNaiveAccel",
    },
  },
  complementary: {
    label: "Complementary",
    color: 0x176b5b,
    toggle: "orientationToggleComplementary",
    fields: {
      pitch: "pitch_complementary_cdeg",
      roll: "roll_complementary_cdeg",
      zenith: "zenith_complementary_cdeg",
      accel: "accel_norm_mg",
    },
    readout: {
      pitch: "orientationComplementaryPitch",
      roll: "orientationComplementaryRoll",
      zenith: "orientationComplementaryZenith",
      accel: "orientationComplementaryAccel",
    },
  },
  mahony: {
    label: "Mahony",
    color: 0xa8482d,
    toggle: "orientationToggleMahony",
    fields: {
      pitch: "pitch_mahony_cdeg",
      roll: "roll_mahony_cdeg",
      zenith: "zenith_mahony_cdeg",
      accel: "accel_norm_mg",
      yaw: "yaw_mahony_cdeg",
    },
    readout: {
      pitch: "orientationMahonyPitch",
      roll: "orientationMahonyRoll",
      zenith: "orientationMahonyZenith",
      accel: "orientationMahonyAccel",
      yaw: "orientationMahonyYaw",
    },
  },
};
const graphConfigs = [
  { enabled: true, metricIds: [], yMode: "auto", yMin: null, yMax: null, xMode: "auto", xSeconds: 30 },
  { enabled: false, metricIds: [], yMode: "auto", yMin: null, yMax: null, xMode: "auto", xSeconds: 30 },
  { enabled: false, metricIds: [], yMode: "auto", yMin: null, yMax: null, xMode: "auto", xSeconds: 30 },
];
const sampleMetadataFields = [
  "host_time_iso", "host_time_ms", "version", "message_type", "stream_id", "flags",
  "sequence", "timestamp_ms", "payload_format", "payload_len",
];
const sampleFooterFields = ["missed_samples"];
const fallbackCapability = {
  streams: [
    {
      stream_id: 1,
      stream_type: "DUMMY_ACCEL3",
      fields: [
        { field: "accel_x_mg", label: "Dummy Accel X", unit: "mg", scale: 1, decimals: 0 },
        { field: "accel_y_mg", label: "Dummy Accel Y", unit: "mg", scale: 1, decimals: 0 },
        { field: "accel_z_mg", label: "Dummy Accel Z", unit: "mg", scale: 1, decimals: 0 },
      ],
      default_interval_ms: 100,
      min_interval_ms: 20,
      max_interval_ms: 10000,
    },
    {
      stream_id: 10,
      stream_type: "IMU6",
      fields: [
        { field: "accel_x_mg", label: "LSM6DSL Accel X", unit: "mg", scale: 1, decimals: 0 },
        { field: "accel_y_mg", label: "LSM6DSL Accel Y", unit: "mg", scale: 1, decimals: 0 },
        { field: "accel_z_mg", label: "LSM6DSL Accel Z", unit: "mg", scale: 1, decimals: 0 },
        { field: "gyro_x_mdps", label: "LSM6DSL Gyro X", unit: "mdps", scale: 1, decimals: 0 },
        { field: "gyro_y_mdps", label: "LSM6DSL Gyro Y", unit: "mdps", scale: 1, decimals: 0 },
        { field: "gyro_z_mdps", label: "LSM6DSL Gyro Z", unit: "mdps", scale: 1, decimals: 0 },
      ],
      default_interval_ms: 38,
      min_interval_ms: 38,
      max_interval_ms: 38,
    },
    {
      stream_id: 13,
      stream_type: "ORIENTATION_MOTION",
      fields: [
        {
          field: "pitch_naive_cdeg",
          label: "Orientation Pitch Naive",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "roll_naive_cdeg",
          label: "Orientation Roll Naive",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "zenith_naive_cdeg",
          label: "Orientation Zenith Naive",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "pitch_complementary_cdeg",
          label: "Orientation Pitch Complementary",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "roll_complementary_cdeg",
          label: "Orientation Roll Complementary",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "zenith_complementary_cdeg",
          label: "Orientation Zenith Complementary",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "pitch_mahony_cdeg",
          label: "Orientation Pitch Mahony",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "roll_mahony_cdeg",
          label: "Orientation Roll Mahony",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "zenith_mahony_cdeg",
          label: "Orientation Zenith Mahony",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "yaw_mahony_cdeg",
          label: "Orientation Yaw Mahony",
          unit: "deg",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "accel_norm_mg",
          label: "Orientation Accel Norm",
          unit: "mg",
          scale: 1,
          decimals: 0,
        },
      ],
      default_interval_ms: 38,
      min_interval_ms: 38,
      max_interval_ms: 38,
    },
    {
      stream_id: 30,
      stream_type: "TEMP_HUMIDITY",
      fields: [
        {
          field: "humidity_centi_percent",
          label: "HTS221 Humidity",
          unit: "%RH",
          scale: 0.01,
          decimals: 2,
        },
        {
          field: "temperature_centi_c",
          label: "HTS221 Temperature",
          unit: "degC",
          scale: 0.01,
          decimals: 2,
        },
      ],
      default_interval_ms: 1000,
      min_interval_ms: 1000,
      max_interval_ms: 1000,
    },
    {
      stream_id: 20,
      stream_type: "PRESSURE",
      fields: [
        { field: "pressure_pa", label: "LPS22HB Pressure", unit: "Pa", scale: 1, decimals: 0 },
      ],
      default_interval_ms: 1000,
      min_interval_ms: 1000,
      max_interval_ms: 1000,
    },
    {
      stream_id: 12,
      stream_type: "MAG3",
      fields: [
        { field: "mag_x_ut", label: "LSM303AGR Mag X", unit: "uT", scale: 1, decimals: 0 },
        { field: "mag_y_ut", label: "LSM303AGR Mag Y", unit: "uT", scale: 1, decimals: 0 },
        { field: "mag_z_ut", label: "LSM303AGR Mag Z", unit: "uT", scale: 1, decimals: 0 },
      ],
      default_interval_ms: 100,
      min_interval_ms: 100,
      max_interval_ms: 100,
    },
  ],
};
let metricDefinitions = {};
let fieldDefinitions = [];
let configurableStreams = [];
let csvValueColumns = [];
let capabilityFromDevice = false;
let connected = false;
let connecting = false;
let socket;
let toastTimer;
let csvRecording = false;
let csvSamples = [];
const timeFormatter = new Intl.DateTimeFormat(undefined, {
  hour: "2-digit",
  minute: "2-digit",
  second: "2-digit",
  hour12: false,
});
const graphElements = Array.from(document.querySelectorAll(".graph-panel")).map((panel) => ({
  panel,
  enabledInput: panel.querySelector('[data-role="graph-enabled"]'),
  metricSelects: panel.querySelector('[data-role="metric-selects"]'),
  yModes: Array.from(panel.querySelectorAll('[data-role="y-mode"]')),
  yMinInput: panel.querySelector('[data-role="y-min"]'),
  yMaxInput: panel.querySelector('[data-role="y-max"]'),
  xModes: Array.from(panel.querySelectorAll('[data-role="x-mode"]')),
  xSecondsInput: panel.querySelector('[data-role="x-seconds"]'),
  canvas: panel.querySelector('[data-role="graph-canvas"]'),
  summary: panel.querySelector('[data-role="graph-summary"]'),
}));
const orientationState = {
  renderer: null,
  scene: null,
  camera: null,
  cuboids: {},
  lastSample: null,
  unavailable: false,
};

async function api(path, options = {}) {
  const response = await fetch(path, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(payload.error || `${response.status} ${response.statusText}`);
  return payload;
}

function showToast(message) {
  elements.toast.textContent = message;
  elements.toast.classList.add("visible");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => elements.toast.classList.remove("visible"), 2800);
}

function updateConnection(value, pending = false) {
  connected = value;
  connecting = pending;
  if (!value && capabilityFromDevice) renderCapability(fallbackCapability, false);
  elements.connectionLabel.textContent = pending ? "Connecting..." : value ? "Connected" : "Disconnected";
  elements.connectButton.textContent = pending ? "Connecting..." : "Connect";
  elements.connectButton.disabled = pending || value || !elements.deviceSelect.value;
  elements.scanButton.disabled = pending;
  elements.deviceSelect.disabled = pending;
  elements.disconnectButton.disabled = !value || pending;
  elements.startButton.disabled = !value || pending;
  elements.stopButton.disabled = !value && !pending;
  elements.stopButton.textContent = pending ? "Cancel connection" : "Stop";
  elements.resetButton.disabled = !value || pending;
  elements.streamSelect.disabled = !value || pending || elements.streamSelect.options.length === 0;
  elements.applyIntervalButton.disabled = !value || pending || elements.streamSelect.options.length === 0;
  elements.applyFilterButton.disabled = !value || pending || !hasOrientationCapability();
  elements.refreshStatusButton.disabled = !value || pending;
}

function updateStatus(status) {
  $("deviceState").textContent = status?.state || "--";
  $("statusInterval").textContent = status ? `${status.sample_interval_ms} ms` : "--";
  $("lastError").textContent = status?.last_error || "--";
  $("lsm6dslError").textContent = status?.optional_sensors?.lsm6dsl?.error || "--";
  $("hts221Error").textContent = status?.optional_sensors?.hts221?.error || "--";
  $("lps22hbError").textContent = status?.optional_sensors?.lps22hb?.error || "--";
  $("lsm303agrMagnError").textContent =
    status?.optional_sensors?.lsm303agr_magn?.error || "--";
  $("connectionCount").textContent = status?.connection_count ?? "--";
  if (status && Number(elements.streamSelect.value) === 1) {
    elements.intervalInput.value = status.sample_interval_ms;
  }
}

function metricId(streamId, field) {
  return `s${streamId}_${field}`;
}

function formatValue(value, definition) {
  const scaled = value * (definition.scale ?? 1);
  const decimals = definition.decimals ?? 0;
  return decimals > 0 ? scaled.toFixed(decimals) : String(Math.round(scaled));
}

function formatAxisValue(value) {
  const magnitude = Math.abs(value);
  if (magnitude >= 1000) return value.toFixed(0);
  if (magnitude >= 100) return value.toFixed(1);
  if (magnitude >= 10) return value.toFixed(2);
  return value.toFixed(3);
}

function compactSignalLabel(label) {
  return label
    .replace(/^LSM6DSL\s+/, "")
    .replace(/^LSM303AGR\s+/, "")
    .replace(/^HTS221\s+/, "")
    .replace(/^LPS22HB\s+/, "")
    .replace(/^Dummy\s+/, "")
    .replace("Temperature", "Temp")
    .replace("Humidity", "Hum")
    .replace("Pressure", "Press");
}

function flattenFields(capability) {
  return (capability.streams || []).flatMap((stream) =>
    (stream.fields || []).map((field) => ({
      streamId: stream.stream_id,
      streamType: stream.stream_type,
      field: field.field,
      label: field.label || `${stream.stream_type} ${field.field}`,
      unit: field.unit || "",
      scale: field.scale ?? 1,
      decimals: field.decimals ?? 0,
      metricId: metricId(stream.stream_id, field.field),
    }))
  );
}

function renderCapability(capability, fromDevice) {
  capabilityFromDevice = fromDevice;
  configurableStreams = (capability.streams || []).filter(
    (stream) => stream.min_interval_ms !== stream.max_interval_ms
  );
  fieldDefinitions = flattenFields(capability);
  metricDefinitions = Object.fromEntries(
    fieldDefinitions.map((definition) => [definition.metricId, definition])
  );
  csvValueColumns = fieldDefinitions.map((definition) => ({
    header: definition.metricId,
    field: definition.field,
    streamId: definition.streamId,
  }));

  elements.metricsGrid.replaceChildren();
  fieldDefinitions.forEach((definition) => {
    const article = document.createElement("article");
    const label = document.createElement("span");
    label.textContent = definition.label;
    const value = document.createElement("strong");
    value.id = `latest_${definition.metricId}`;
    value.textContent = "--";
    const unit = document.createElement("small");
    unit.textContent = definition.unit;
    article.append(label, value, unit);
    elements.metricsGrid.append(article);
  });

  renderStreamControls();
  renderGraphSignalControls();
  elements.applyFilterButton.disabled = !connected || !hasOrientationCapability();
  updateOrientationView();
  drawCharts();
}

function streamLabel(stream) {
  return `${stream.stream_id}: ${stream.stream_type}`;
}

function renderStreamControls() {
  const previous = elements.streamSelect.value;
  const previousInterval = elements.intervalInput.value;
  elements.streamSelect.replaceChildren();
  configurableStreams.forEach((stream) => {
    elements.streamSelect.add(new Option(streamLabel(stream), String(stream.stream_id)));
  });
  const previousStillSelectable = configurableStreams.some(
    (stream) => String(stream.stream_id) === previous
  );
  if (previousStillSelectable) {
    elements.streamSelect.value = previous;
  }
  const selected = configurableStreams.find(
    (stream) => String(stream.stream_id) === elements.streamSelect.value
  ) || configurableStreams[0];
  if (selected) {
    elements.streamSelect.value = String(selected.stream_id);
    elements.intervalInput.min = selected.min_interval_ms;
    elements.intervalInput.max = selected.max_interval_ms;
    if (!previousStillSelectable || previousInterval === "") {
      elements.intervalInput.value = selected.default_interval_ms;
    }
  }
  elements.streamSelect.disabled = !connected || elements.streamSelect.options.length === 0;
  elements.applyIntervalButton.disabled = !connected || elements.streamSelect.options.length === 0;
}

function uniqueMetricIds(ids) {
  const seen = new Set();
  return ids.filter((id) => {
    if (!id || seen.has(id) || !metricDefinitions[id]) return false;
    seen.add(id);
    return true;
  }).slice(0, maxGraphSignals);
}

function makeMetricSelect(graphIndex, signalIndex, selectedMetric) {
  const select = document.createElement("select");
  select.dataset.role = "metric-select";
  select.ariaLabel = `Graph ${graphIndex + 1} signal ${signalIndex + 1}`;
  select.add(new Option(`Signal ${signalIndex + 1}: none`, ""));
  fieldDefinitions.forEach((definition) => {
    select.add(new Option(definition.label, definition.metricId));
  });
  if (metricDefinitions[selectedMetric]) select.value = selectedMetric;
  select.addEventListener("change", () => {
    graphConfigs[graphIndex].metricIds = uniqueMetricIds(
      Array.from(graphElements[graphIndex].metricSelects.querySelectorAll("select")).map(
        (input) => input.value
      )
    );
    syncGraphSelectValues(graphIndex);
    drawCharts();
  });
  return select;
}

function renderGraphSignalControls() {
  graphElements.forEach(({ metricSelects }, graphIndex) => {
    const config = graphConfigs[graphIndex];
    config.metricIds = uniqueMetricIds(config.metricIds);
    if (graphIndex === 0 && config.metricIds.length === 0 && fieldDefinitions[0]) {
      config.metricIds = [fieldDefinitions[0].metricId];
    }

    metricSelects.replaceChildren();
    for (let signalIndex = 0; signalIndex < maxGraphSignals; signalIndex += 1) {
      metricSelects.append(makeMetricSelect(graphIndex, signalIndex, config.metricIds[signalIndex] || ""));
    }
    updateGraphControlState(graphIndex);
  });
}

function syncGraphSelectValues(graphIndex) {
  const selects = Array.from(graphElements[graphIndex].metricSelects.querySelectorAll("select"));
  selects.forEach((select, index) => {
    select.value = graphConfigs[graphIndex].metricIds[index] || "";
  });
}

function numericInputValue(input) {
  const value = Number(input.value);
  return Number.isFinite(value) ? value : null;
}

function syncGraphConfig(index) {
  const graph = graphElements[index];
  const config = graphConfigs[index];
  config.enabled = graph.enabledInput.checked;
  config.metricIds = uniqueMetricIds(
    Array.from(graph.metricSelects.querySelectorAll("select")).map((select) => select.value)
  );
  config.yMode = graph.yModes.find((input) => input.checked)?.value || "auto";
  config.yMin = numericInputValue(graph.yMinInput);
  config.yMax = numericInputValue(graph.yMaxInput);
  config.xMode = graph.xModes.find((input) => input.checked)?.value || "auto";
  config.xSeconds = Math.max(1, numericInputValue(graph.xSecondsInput) || 30);
}

function updateGraphControlState(index) {
  const graph = graphElements[index];
  const config = graphConfigs[index];
  const disabled = !config.enabled;
  graph.panel.classList.toggle("disabled", disabled);
  Array.from(graph.metricSelects.querySelectorAll("select")).forEach((select) => {
    select.disabled = disabled;
  });
  graph.yModes.forEach((input) => {
    input.disabled = disabled;
  });
  graph.xModes.forEach((input) => {
    input.disabled = disabled;
  });
  graph.yMinInput.disabled = disabled || config.yMode !== "manual";
  graph.yMaxInput.disabled = disabled || config.yMode !== "manual";
  graph.xSecondsInput.disabled = disabled || config.xMode !== "seconds";
}

async function refreshCapability() {
  if (!connected) {
    renderCapability(fallbackCapability, false);
    return;
  }
  try {
    const payload = await api("/api/capability");
    renderCapability(payload.capability, true);
  } catch (error) {
    showToast(error.message);
  }
}

function sampleMatchesStream(sample, streamId) {
  return streamId === undefined || sample.stream_id === streamId;
}

function numericValue(sample, field, streamId) {
  if (!sampleMatchesStream(sample, streamId)) return null;
  const value = sample[field];
  if (value === null || value === undefined || value === "") return null;
  const number = Number(value);
  return Number.isFinite(number) ? number : null;
}

function sameSample(left, right) {
  return left?.stream_id === right.stream_id
    && left?.sequence === right.sequence
    && left?.timestamp_ms === right.timestamp_ms
    && left?.payload_format === right.payload_format;
}

function scaledNumericValue(sample, definition) {
  const value = numericValue(sample, definition.field, definition.streamId);
  return value === null ? null : value * (definition.scale ?? 1);
}

function updateMetric(definition, sample) {
  const { field, streamId } = definition;
  const value = numericValue(sample, field, streamId);
  if (value !== null) $(`latest_${definition.metricId}`).textContent = formatValue(value, definition);
}

function hasOrientationCapability() {
  return fieldDefinitions.some((definition) => definition.streamId === 13);
}

function setupOrientationScene() {
  if (!elements.orientationCanvas || orientationState.renderer || orientationState.unavailable) return;
  try {
    const renderer = new THREE.WebGLRenderer({
      canvas: elements.orientationCanvas,
      antialias: true,
      preserveDrawingBuffer: true,
    });
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0xf8fafb);
    const camera = new THREE.PerspectiveCamera(35, 1, 0.1, 100);
    camera.position.set(4.6, 2.5, 5.8);
    camera.lookAt(0, 0, 0);

    const positions = { naive: -2.6, complementary: 0, mahony: 2.6 };
    Object.entries(orientationModes).forEach(([key, mode]) => {
      const geometry = new THREE.BoxGeometry(1.85, 0.28, 1.05);
      const cuboid = new THREE.Mesh(
        geometry,
        [
          new THREE.MeshStandardMaterial({ color: mode.color, roughness: 0.56 }),
          new THREE.MeshStandardMaterial({ color: mode.color, roughness: 0.56 }),
          new THREE.MeshStandardMaterial({ color: 0xb7c4cc, roughness: 0.62 }),
          new THREE.MeshStandardMaterial({ color: 0xf0f4f6, roughness: 0.62 }),
          new THREE.MeshStandardMaterial({ color: 0xf2c94c, roughness: 0.52 }),
          new THREE.MeshStandardMaterial({ color: 0x26333d, roughness: 0.52 }),
        ]
      );
      cuboid.position.x = positions[key];
      cuboid.userData.baseX = positions[key];
      cuboid.add(new THREE.LineSegments(
        new THREE.EdgesGeometry(geometry),
        new THREE.LineBasicMaterial({ color: 0x26333d })
      ));
      scene.add(cuboid);
      orientationState.cuboids[key] = cuboid;
    });
    scene.add(new THREE.HemisphereLight(0xffffff, 0xb7c4cc, 2.5));
    const keyLight = new THREE.DirectionalLight(0xffffff, 2.6);
    keyLight.position.set(3, 5, 4);
    scene.add(keyLight);
    const grid = new THREE.GridHelper(7.4, 8, 0xcfd8de, 0xe4eaee);
    grid.position.y = -0.72;
    scene.add(grid);

    orientationState.renderer = renderer;
    orientationState.scene = scene;
    orientationState.camera = camera;
  } catch (error) {
    orientationState.unavailable = true;
    elements.orientationState.textContent = "3D renderer unavailable";
  }
}

function renderOrientationScene() {
  setupOrientationScene();
  if (!orientationState.renderer) return;
  const canvas = elements.orientationCanvas;
  const width = Math.max(320, Math.floor(canvas.clientWidth));
  const height = Math.max(240, Math.floor(canvas.clientHeight));
  orientationState.renderer.setPixelRatio(window.devicePixelRatio || 1);
  orientationState.renderer.setSize(width, height, false);
  orientationState.camera.aspect = width / height;
  orientationState.camera.updateProjectionMatrix();
  orientationState.renderer.render(orientationState.scene, orientationState.camera);
}

function orientationRawValue(sample, field) {
  return sample ? numericValue(sample, field, 13) : null;
}

function orientationScaledValue(sample, field) {
  const raw = orientationRawValue(sample, field);
  return raw === null ? null : raw * 0.01;
}

function orientationModeValues(sample, mode) {
  const config = orientationModes[mode];
  return {
    pitch: orientationScaledValue(sample, config.fields.pitch),
    roll: orientationScaledValue(sample, config.fields.roll),
    zenith: orientationScaledValue(sample, config.fields.zenith),
    accel: orientationRawValue(sample, config.fields.accel),
    yaw: config.fields.yaw ? orientationScaledValue(sample, config.fields.yaw) : null,
  };
}

function setReadoutText(id, value, decimals = 2) {
  elements[id].textContent = value === null ? "--" : value.toFixed(decimals);
}

function setOrientationReadout(source) {
  Object.entries(orientationModes).forEach(([mode, config]) => {
    const values = source ? orientationModeValues(source, mode) : {
      pitch: null,
      roll: null,
      zenith: null,
      accel: null,
      yaw: null,
    };
    setReadoutText(config.readout.pitch, values.pitch);
    setReadoutText(config.readout.roll, values.roll);
    setReadoutText(config.readout.zenith, values.zenith);
    setReadoutText(config.readout.accel, values.accel, 0);
    if (config.readout.yaw) setReadoutText(config.readout.yaw, values.yaw);
  });
}

function enabledOrientationModes() {
  return Object.entries(orientationModes)
    .filter(([_mode, config]) => elements[config.toggle].checked)
    .map(([mode]) => mode);
}

function updateOrientationCards() {
  Object.entries(orientationModes).forEach(([mode, config]) => {
    const card = document.querySelector(`[data-orientation-card="${mode}"]`);
    if (card) card.hidden = !elements[config.toggle].checked;
  });
}

function applyCuboidPose(mode, values, visibleIndex, visibleCount) {
  const cuboid = orientationState.cuboids[mode];
  if (!cuboid) return;
  const enabled = values.pitch !== null && values.roll !== null && elements[orientationModes[mode].toggle].checked;
  cuboid.visible = enabled;
  if (!enabled) return;
  const spacing = 2.6;
  cuboid.position.x = (visibleIndex - ((visibleCount - 1) / 2)) * spacing;
  cuboid.rotation.x = THREE.MathUtils.degToRad(values.roll);
  cuboid.rotation.y = values.yaw === null ? 0 : THREE.MathUtils.degToRad(values.yaw);
  cuboid.rotation.z = THREE.MathUtils.degToRad(-values.pitch);
}

function updateOrientationView(sample = null) {
  if (!elements.orientationCanvas) return;
  if (sample?.stream_id === 13) orientationState.lastSample = sample;
  const source = sample?.stream_id === 13 ? sample : orientationState.lastSample;
  updateOrientationCards();

  if (!hasOrientationCapability()) {
    elements.orientationState.textContent = "Orientation stream unavailable";
    elements.orientationMode.textContent = "--";
    setOrientationReadout(null);
    renderOrientationScene();
    return;
  }

  if (!source) {
    elements.orientationState.textContent = "Waiting for orientation sample";
    elements.orientationMode.textContent = "Waiting";
    setOrientationReadout(null);
    renderOrientationScene();
    return;
  }

  const visibleModes = enabledOrientationModes();
  setOrientationReadout(source);
  elements.orientationMode.textContent = `${visibleModes.length} visible`;
  elements.orientationState.textContent = `Orientation sample #${source.sequence ?? "--"}`;

  setupOrientationScene();
  visibleModes.forEach((mode, index) => {
    applyCuboidPose(mode, orientationModeValues(source, mode), index, visibleModes.length);
  });
  Object.keys(orientationModes)
    .filter((mode) => !visibleModes.includes(mode))
    .forEach((mode) => {
      if (orientationState.cuboids[mode]) orientationState.cuboids[mode].visible = false;
    });
  renderOrientationScene();
}

async function scan() {
  elements.scanButton.disabled = true;
  try {
    const { devices } = await api("/api/scan");
    elements.deviceSelect.replaceChildren();
    if (!devices.length) {
      elements.deviceSelect.add(new Option("No devices found", ""));
    }
    devices.forEach((device) => {
      elements.deviceSelect.add(new Option(`${device.name} · ${device.address}`, device.address));
    });
    elements.connectButton.disabled = connected || !elements.deviceSelect.value;
    showToast(`${devices.length} device${devices.length === 1 ? "" : "s"} found`);
  } catch (error) {
    showToast(error.message);
  } finally {
    elements.scanButton.disabled = false;
  }
}

async function command(path, body) {
  try {
    await api(path, { method: "POST", body: JSON.stringify(body || {}) });
  } catch (error) {
    showToast(error.message);
    throw error;
  }
}

function updateSample(sample) {
  const previous = history.at(-1);
  if (previous && sameSample(previous, sample)) return;
  history.push(sample);
  if (history.length > maxHistoryPoints) history.splice(0, history.length - maxHistoryPoints);
  fieldDefinitions.forEach((definition) => updateMetric(definition, sample));
  $("sampleCount").textContent = history.length;
  $("missedCount").textContent = sample.missed_samples;
  updateOrientationView(sample);
  if (csvRecording) {
    csvSamples.push(sample);
    elements.csvState.textContent = `Recording · ${csvSamples.length}`;
  }
  drawCharts();
}

async function refreshStatus() {
  try {
    const payload = await api("/api/status");
    updateConnection(payload.connected, payload.connecting);
    if (payload.connected && !capabilityFromDevice) await refreshCapability();
    if (payload.latest_sample) updateSample(payload.latest_sample);
    updateStatus(payload.status);
  } catch (error) {
    showToast(error.message);
  }
}

async function commandAndRefresh(path, body) {
  await command(path, body);
  await new Promise((resolve) => setTimeout(resolve, 100));
  await refreshStatus();
}

function boundedNumber(input, fallback) {
  const value = Number(input.value);
  if (!Number.isFinite(value)) return fallback;
  const min = Number(input.min);
  const max = Number(input.max);
  return Math.min(max, Math.max(min, value));
}

async function applyOrientationFilters() {
  const body = {
    complementary_alpha: boundedNumber(elements.complementaryAlphaInput, 0.98),
    mahony_kp: boundedNumber(elements.mahonyKpInput, 0.5),
    mahony_ki: boundedNumber(elements.mahonyKiInput, 0),
  };
  elements.complementaryAlphaInput.value = body.complementary_alpha;
  elements.mahonyKpInput.value = body.mahony_kp;
  elements.mahonyKiInput.value = body.mahony_ki;
  await commandAndRefresh("/api/orientation-filter", body);
  showToast("Orientation filters applied");
}

function connectSocket() {
  const scheme = location.protocol === "https:" ? "wss" : "ws";
  socket = new WebSocket(`${scheme}://${location.host}/ws`);
  socket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    if (message.type === "sample") updateSample(message.sample);
    if (message.type === "state") updateConnection(message.connected, message.connecting);
  };
  socket.onclose = () => setTimeout(connectSocket, 1000);
}

function setupCanvas(canvas) {
  const ratio = window.devicePixelRatio || 1;
  const width = Math.max(320, canvas.clientWidth);
  const height = Math.max(220, canvas.clientHeight);
  if (canvas.width !== width * ratio || canvas.height !== height * ratio) {
    canvas.width = width * ratio;
    canvas.height = height * ratio;
  }
  const ctx = canvas.getContext("2d");
  ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
  ctx.clearRect(0, 0, width, height);
  return { ctx, width, height };
}

function drawCanvasMessage(ctx, width, height, message) {
  ctx.fillStyle = "#65737d";
  ctx.font = "13px system-ui";
  ctx.textAlign = "center";
  ctx.fillText(message, width / 2, height / 2);
  ctx.textAlign = "start";
}

function graphMetrics(index) {
  return graphConfigs[index].metricIds.map((id) => metricDefinitions[id]).filter(Boolean).slice(0, 3);
}

function graphPointSeries(metrics, xLow, xHigh) {
  return metrics.map((metric) => ({
    metric,
    points: history
      .map((sample) => ({
        time: Number(sample.host_time_ms),
        value: scaledNumericValue(sample, metric),
      }))
      .filter((point) =>
        Number.isFinite(point.time)
        && point.value !== null
        && point.time >= xLow
        && point.time <= xHigh
      ),
  }));
}

function selectedPointTimes(metrics) {
  const times = [];
  history.forEach((sample) => {
    const time = Number(sample.host_time_ms);
    if (!Number.isFinite(time)) return;
    if (metrics.some((metric) => scaledNumericValue(sample, metric) !== null)) times.push(time);
  });
  return times;
}

function xDomain(config, metrics) {
  if (config.xMode === "seconds") {
    const latest = Number(history.at(-1)?.host_time_ms) || Date.now();
    return { low: latest - config.xSeconds * 1000, high: latest };
  }
  const times = selectedPointTimes(metrics);
  if (!times.length) {
    const latest = Number(history.at(-1)?.host_time_ms) || Date.now();
    return { low: latest - 1000, high: latest };
  }
  const low = Math.min(...times);
  const high = Math.max(...times);
  if (low === high) return { low: low - 1000, high: high + 1000 };
  return { low, high };
}

function yDomain(config, series) {
  if (config.yMode === "manual" && config.yMin !== null && config.yMax !== null && config.yMax > config.yMin) {
    return { low: config.yMin, high: config.yMax };
  }
  const values = series.flatMap((item) => item.points.map((point) => point.value));
  if (!values.length) return { low: 0, high: 1 };
  const min = Math.min(...values);
  const max = Math.max(...values);
  if (min === max) {
    const pad = Math.max(1, Math.abs(min) * 0.1);
    return { low: min - pad, high: max + pad };
  }
  const range = max - min;
  return { low: min - range * 0.12, high: max + range * 0.12 };
}

function drawGrid(ctx, width, height, pad, yLow, yHigh, xLow, xHigh) {
  const plotWidth = width - pad.left - pad.right;
  const plotHeight = height - pad.top - pad.bottom;
  ctx.strokeStyle = "#dce3e7";
  ctx.fillStyle = "#65737d";
  ctx.font = "12px system-ui";
  ctx.lineWidth = 1;
  for (let i = 0; i <= 4; i += 1) {
    const y = pad.top + (plotHeight * i) / 4;
    ctx.beginPath();
    ctx.moveTo(pad.left, y);
    ctx.lineTo(width - pad.right, y);
    ctx.stroke();
    const label = yHigh - ((yHigh - yLow) * i) / 4;
    ctx.fillText(formatAxisValue(label), 8, y + 4);
  }

  ctx.textAlign = "center";
  const tickCount = Math.max(2, Math.min(5, Math.floor(plotWidth / 90)));
  for (let i = 0; i < tickCount; i += 1) {
    const ratio = tickCount === 1 ? 0 : i / (tickCount - 1);
    const x = pad.left + plotWidth * ratio;
    const time = xLow + (xHigh - xLow) * ratio;
    ctx.beginPath();
    ctx.moveTo(x, pad.top);
    ctx.lineTo(x, pad.top + plotHeight);
    ctx.stroke();
    ctx.fillText(timeFormatter.format(new Date(time)), x, height - 12);
  }
  ctx.textAlign = "start";
}

function drawGraph(index) {
  syncGraphConfig(index);
  updateGraphControlState(index);
  const graph = graphElements[index];
  const config = graphConfigs[index];
  const { ctx, width, height } = setupCanvas(graph.canvas);

  if (!config.enabled) {
    graph.summary.textContent = "Disabled";
    drawCanvasMessage(ctx, width, height, "Graph disabled");
    return;
  }

  const metrics = graphMetrics(index);
  if (!metrics.length) {
    graph.summary.textContent = "No signal";
    drawCanvasMessage(ctx, width, height, "Select up to 3 signals");
    return;
  }

  const fullSummary = metrics.map((metric) => metric.label).join(" / ");
  graph.summary.textContent = metrics.map((metric) => compactSignalLabel(metric.label)).join(" / ");
  graph.summary.title = fullSummary;
  const pad = { left: 62, right: 18, top: 18, bottom: 42 };
  const plotWidth = width - pad.left - pad.right;
  const plotHeight = height - pad.top - pad.bottom;
  const { low: xLow, high: xHigh } = xDomain(config, metrics);
  const series = graphPointSeries(metrics, xLow, xHigh);
  const { low: yLow, high: yHigh } = yDomain(config, series);
  drawGrid(ctx, width, height, pad, yLow, yHigh, xLow, xHigh);

  const denominatorX = Math.max(1, xHigh - xLow);
  const denominatorY = Math.max(1, yHigh - yLow);
  series.forEach((item, seriesIndex) => {
    if (item.points.length < 2) return;
    ctx.strokeStyle = graphColors[seriesIndex % graphColors.length];
    ctx.lineWidth = 2;
    ctx.beginPath();
    item.points.forEach((point, pointIndex) => {
      const x = pad.left + ((point.time - xLow) / denominatorX) * plotWidth;
      const y = pad.top + plotHeight - ((point.value - yLow) / denominatorY) * plotHeight;
      if (pointIndex === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    });
    ctx.stroke();

    const lastPoint = item.points.at(-1);
    if (lastPoint) {
      const x = pad.left + ((lastPoint.time - xLow) / denominatorX) * plotWidth;
      const y = pad.top + plotHeight - ((lastPoint.value - yLow) / denominatorY) * plotHeight;
      ctx.fillStyle = graphColors[seriesIndex % graphColors.length];
      ctx.beginPath();
      ctx.arc(x, y, 3, 0, Math.PI * 2);
      ctx.fill();
    }
  });

  if (series.every((item) => item.points.length === 0)) {
    drawCanvasMessage(ctx, width, height, "Waiting for selected stream samples");
  }
}

function drawCharts() {
  graphElements.forEach((_graph, index) => drawGraph(index));
}

function csvCell(value) {
  const text = String(value ?? "");
  return /[",\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
}

function saveCsv() {
  if (!csvSamples.length) {
    showToast("No samples were recorded");
    return;
  }
  const fields = [
    ...sampleMetadataFields,
    ...csvValueColumns.map((column) => column.header),
    ...sampleFooterFields,
  ];
  const rows = csvSamples.map((sample) => [
    new Date(sample.host_time_ms).toISOString(),
    ...sampleMetadataFields.slice(1).map((field) => sample[field]),
    ...csvValueColumns.map((column) =>
      sampleMatchesStream(sample, column.streamId) ? sample[column.field] : null
    ),
    ...sampleFooterFields.map((field) => sample[field]),
  ]);
  const csv = [fields, ...rows].map((row) => row.map(csvCell).join(",")).join("\r\n");
  const blob = new Blob([`\uFEFF${csv}\r\n`], { type: "text/csv;charset=utf-8" });
  const link = document.createElement("a");
  const stamp = new Date().toISOString().replaceAll(":", "-").replace(/\.\d{3}Z$/, "Z");
  link.href = URL.createObjectURL(blob);
  link.download = `ble_sensor_log_${stamp}.csv`;
  document.body.append(link);
  link.click();
  link.remove();
  setTimeout(() => URL.revokeObjectURL(link.href), 1000);
  showToast(`${csvSamples.length} samples saved`);
}

function toggleCsvRecording() {
  if (!csvRecording) {
    csvSamples = [];
    csvRecording = true;
    elements.csvButton.textContent = "Stop & save CSV";
    elements.csvButton.classList.remove("secondary");
    elements.csvState.textContent = "Recording · 0";
    elements.csvState.classList.add("active");
    return;
  }
  csvRecording = false;
  elements.csvButton.textContent = "Start CSV log";
  elements.csvButton.classList.add("secondary");
  elements.csvState.textContent = `Saved · ${csvSamples.length}`;
  elements.csvState.classList.remove("active");
  saveCsv();
}

elements.scanButton.addEventListener("click", scan);
elements.deviceSelect.addEventListener("change", () => {
  elements.connectButton.disabled = connected || !elements.deviceSelect.value;
});
elements.connectButton.addEventListener("click", async () => {
  updateConnection(false, true);
  try {
    const result = await api("/api/connect", {
      method: "POST",
      body: JSON.stringify({ target: elements.deviceSelect.value }),
    });
    if (!result.cancelled) {
      await refreshStatus();
      await refreshCapability();
    }
  } catch (error) {
    showToast(error.message);
    updateConnection(false);
  }
});
elements.disconnectButton.addEventListener("click", async () => {
  await command("/api/disconnect");
  updateConnection(false);
  updateStatus(null);
});
elements.startButton.addEventListener("click", () => commandAndRefresh("/api/start"));
elements.stopButton.addEventListener("click", async () => {
  if (connecting) {
    await command("/api/connect/cancel");
    updateConnection(false);
    showToast("Connection cancelled");
    return;
  }
  await commandAndRefresh("/api/stop");
});
elements.resetButton.addEventListener("click", async () => {
  await command("/api/reset-sequence");
  history.length = 0;
  drawCharts();
});
elements.streamSelect.addEventListener("change", renderStreamControls);
elements.applyIntervalButton.addEventListener("click", () =>
  commandAndRefresh("/api/interval", {
    stream_id: Number(elements.streamSelect.value),
    interval_ms: Number(elements.intervalInput.value),
  })
);
elements.applyFilterButton.addEventListener("click", applyOrientationFilters);
elements.refreshStatusButton.addEventListener("click", refreshStatus);
elements.clearButton.addEventListener("click", () => {
  history.length = 0;
  orientationState.lastSample = null;
  $("sampleCount").textContent = "0";
  updateOrientationView();
  drawCharts();
});
elements.csvButton.addEventListener("click", toggleCsvRecording);
[
  elements.orientationToggleNaive,
  elements.orientationToggleComplementary,
  elements.orientationToggleMahony,
].forEach((input) => input.addEventListener("change", updateOrientationView));
graphElements.forEach((graph, index) => {
  const redraw = () => {
    syncGraphConfig(index);
    updateGraphControlState(index);
    drawCharts();
  };
  graph.enabledInput.addEventListener("change", redraw);
  graph.yModes.forEach((input) => input.addEventListener("change", redraw));
  graph.xModes.forEach((input) => input.addEventListener("change", redraw));
  graph.yMinInput.addEventListener("input", redraw);
  graph.yMaxInput.addEventListener("input", redraw);
  graph.xSecondsInput.addEventListener("input", redraw);
});
window.addEventListener("resize", () => {
  drawCharts();
  renderOrientationScene();
});

updateConnection(false);
renderCapability(fallbackCapability, false);
connectSocket();
refreshStatus();
drawCharts();
renderOrientationScene();

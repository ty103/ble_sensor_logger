cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_sensor_logger)

target_sources(app PRIVATE
  src/main.c
  src/protocol/protocol.c
  src/modules/ble_service.c
  src/modules/control.c
  src/modules/hts221_sensor.c
  src/modules/lps22hb_sensor.c
  src/modules/lsm303agr_magn_sensor.c
  src/modules/lsm6dsl_sensor.c
  src/modules/sensor_dummy.c
  src/platform/platform_time.c
  src/events/ble_events.c
  src/events/config_events.c
  src/events/sensor_events.c
  src/events/status_events.c
)

target_sources_ifdef(CONFIG_BSL_IKS01A2_I2C_PROBE app PRIVATE
  src/modules/iks01a2_i2c_probe.c
)

target_include_directories(app PRIVATE
  src
  src/events
  src/modules
  src/platform
  src/protocol
)

target_link_libraries(app PRIVATE m)

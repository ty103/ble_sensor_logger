#!/bin/bash
# RTT log capture automation for nRF52840 + JLinkRTTLogger
#
# This script automates the process of:
#   1. Optionally flash the firmware
#   2. Capture RTT log from channel 0 using JLinkRTTLogger
#   3. Reset the device if needed
#   4. Save output to a log file
#
# Usage:
#   ./firmware/scripts/rtt_capture.sh [OPTIONS]
#
# Options:
#   --flash                  Flash firmware before capturing RTT log
#   --build-dir DIR          Build directory (default: build)
#   --serial SN              J-Link serial number (auto-detect if omitted)
#   --channel N              RTT channel (default: 0)
#   --ready-timeout SEC      Timeout to wait for RTT ready (default: 0.8)
#   --capture-after-reset SEC Window to capture after reset (default: 0.8)
#   --help                   Show this help
#
# Environment:
#   Before running, ensure: source firmware/.env
#
# Examples:
#   # Basic RTT capture
#   ./firmware/scripts/rtt_capture.sh --build-dir build --serial 1050278440
#
#   # Flash and capture
#   ./firmware/scripts/rtt_capture.sh --flash --build-dir build --serial 1050278440
#
#   # Auto-detect device
#   ./firmware/scripts/rtt_capture.sh --flash --build-dir build
#
# Output:
#   - Log file: build/rtt_capture.log
#   - Status: "RESULT: ok=True" on success
#
# See: ../docs/runbooks/rtt_debug.md for detailed guide

set -euo pipefail

# Show help
if [[ "${1:-}" == "--help" ]] || [[ "${1:-}" == "-h" ]]; then
  grep '^#' "$0" | sed 's/^# //'
  exit 0
fi

# Defaults
SERIAL="${SERIAL:-}"
CHANNEL="${CHANNEL:-0}"
READY_TIMEOUT="${READY_TIMEOUT:-0.8}"
CAPTURE_AFTER_RESET="${CAPTURE_AFTER_RESET:-0.8}"
BUILD_DIR="${BUILD_DIR:-build}"
DO_FLASH=false
LOG_PATH="build/rtt_capture.log"

# Auto-detect serial if not specified
detect_serial() {
  local devices
  devices=$(nrfutil device list 2>/dev/null | grep -E '^[0-9a-f]+$' || true)
  local count
  count=$(echo "$devices" | wc -l | tr -d ' ')
  
  if [[ $count -eq 0 ]]; then
    echo "ERROR: no J-Link devices found"
    return 1
  elif [[ $count -eq 1 ]]; then
    echo "$devices" | head -n 1
    return 0
  else
    echo "ERROR: multiple J-Link devices found, specify --serial:"
    echo "$devices"
    return 1
  fi
}

# Parse arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    --flash)
      DO_FLASH=true
      shift
      ;;
    --serial)
      SERIAL="$2"
      shift 2
      ;;
    --channel)
      CHANNEL="$2"
      shift 2
      ;;
    --ready-timeout)
      READY_TIMEOUT="$2"
      shift 2
      ;;
    --capture-after-reset)
      CAPTURE_AFTER_RESET="$2"
      shift 2
      ;;
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    --help|-h)
      grep '^#' "$0" | sed 's/^# //'
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      exit 2
      ;;
  esac
done

# Auto-detect serial if not specified
if [[ -z "$SERIAL" ]]; then
  SERIAL=$(detect_serial)
  if [[ $? -ne 0 ]]; then
    exit 1
  fi
  echo "AUTO_DETECTED_SERIAL: $SERIAL"
fi

# Flash if requested
if [[ "$DO_FLASH" == "true" ]]; then
  echo "FLASH_CMD: west flash --build-dir $BUILD_DIR --dev-id $SERIAL"
  west flash --build-dir "$BUILD_DIR" --dev-id "$SERIAL"
  if [[ $? -ne 0 ]]; then
    echo "ERROR: flash failed"
    exit 1
  fi
fi

# Remove old log
[[ -f "$LOG_PATH" ]] && rm -f "$LOG_PATH"

echo "ATTEMPT: 1/1"
START=$(date +%s%N)

# Run JLinkRTTLogger in background with controlled input
# Inputs:
#   device:           NRF52840_XXAA
#   interface:        (default SWD)
#   speed:            4000 kHz
#   RTT address:      (auto)
#   channel:          (from CHANNEL var)
#   output file:      build/rtt_capture.log
#
# Then:
#   - Wait for RTT ready
#   - Trigger device reset via nrfutil
#   - Capture window
#   - Exit

{
  sleep 0.2
  echo 'NRF52840_XXAA'  # device name
  echo ''               # interface (default SWD)
  echo '4000'           # speed kHz
  echo ''               # RTT address (auto)
  echo "$CHANNEL"       # channel
  echo 'build/rtt_capture.log'  # output file
  sleep "$READY_TIMEOUT"
  # Trigger reset
  nrfutil device reset --serial-number "$SERIAL" 2>/dev/null || true
  # Capture window
  sleep "$CAPTURE_AFTER_RESET"
  echo ''      # Press any key to quit
} | JLinkRTTLogger &

JLINK_PID=$!
LOGGER_EXIT=0

# Kill after 3 seconds max to ensure we don't hang
sleep 3
if kill -0 $JLINK_PID 2>/dev/null; then
  kill -TERM $JLINK_PID 2>/dev/null || true
  sleep 0.5
  kill -KILL $JLINK_PID 2>/dev/null || true
fi

wait $JLINK_PID 2>/dev/null || LOGGER_EXIT=$?
echo "LOGGER_EXIT: $LOGGER_EXIT"

# Check if log exists
if [[ ! -f "$LOG_PATH" ]]; then
  echo "ERROR: log file not found: $LOG_PATH"
  exit 1
fi

LOG_SIZE=$(stat -f%z "$LOG_PATH" 2>/dev/null || stat -c%s "$LOG_PATH" 2>/dev/null || echo 0)
echo "LOG_SIZE_BYTES: $LOG_SIZE"

# Extract first non-empty line
FIRST_LINE=$(grep -v '^[[:space:]]*$' "$LOG_PATH" | head -n 1 || true)
if [[ -z "$FIRST_LINE" ]]; then
  echo "ERROR: no non-empty first line found"
  exit 1
fi

END=$(date +%s%N)
ELAPSED_MS=$(( (END - START) / 1000000 ))

echo "RESULT: ok=True reason=captured log=$LOG_PATH"
echo "ELAPSED_SEC: $(printf "%.3f" $(echo "scale=3; $ELAPSED_MS / 1000" | bc))"
echo "FIRST_LINE: $FIRST_LINE"

exit 0

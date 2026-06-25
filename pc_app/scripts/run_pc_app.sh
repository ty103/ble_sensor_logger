#!/usr/bin/env bash
set -euo pipefail

HOST="127.0.0.1"
PORT="8765"

# Return unique PIDs listening on the target port.
get_listening_pids() {
	if command -v lsof >/dev/null 2>&1; then
		lsof -tiTCP:"${PORT}" -sTCP:LISTEN 2>/dev/null || true
		return
	fi

	if command -v ss >/dev/null 2>&1; then
		ss -ltnp "sport = :${PORT}" 2>/dev/null \
			| grep -o 'pid=[0-9]\+' \
			| cut -d= -f2 \
			| sort -u || true
		return
	fi

	echo "Warning: neither lsof nor ss is available. Skip port cleanup." >&2
}

PIDS="$(get_listening_pids)"

if [[ -n "${PIDS}" ]]; then
	echo "Port ${PORT} is in use. Stopping process(es): ${PIDS}"
	kill ${PIDS} 2>/dev/null || true

	# Force kill if processes are still listening after TERM.
	sleep 1
	REMAINING_PIDS="$(get_listening_pids)"
	if [[ -n "${REMAINING_PIDS}" ]]; then
		echo "Force killing process(es): ${REMAINING_PIDS}"
		kill -9 ${REMAINING_PIDS} 2>/dev/null || true
	fi

	echo "Stopped process(es) on port ${PORT}."
fi

echo "Starting app on ${HOST}:${PORT} ..."
uv run --extra dev python -m ble_sensor_logger --web --host "${HOST}" --port "${PORT}"
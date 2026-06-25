@echo off
setlocal EnableExtensions

set "HOST=127.0.0.1"
set "PORT=8765"

call "%~dp0stop_pc_app.bat"

echo Starting app on %HOST%:%PORT% ...
uv run --extra dev python -m ble_sensor_logger --web --host %HOST% --port %PORT%
exit /b %ERRORLEVEL%

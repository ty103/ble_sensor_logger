@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PORT=8765"
set "PIDS="
set "REMAINING="

call :get_listening_pids PIDS

if not defined PIDS (
    echo No process is listening on port %PORT%.
    exit /b 0
)

echo Port %PORT% is in use. Stopping process(es): %PIDS%
for %%P in (%PIDS%) do taskkill /PID %%P >nul 2>&1

timeout /t 1 /nobreak >nul
call :get_listening_pids REMAINING
if defined REMAINING (
    echo Force killing process(es): %REMAINING%
    for %%P in (%REMAINING%) do taskkill /F /PID %%P >nul 2>&1
)

echo Stopped process(es) on port %PORT%.
exit /b 0

:get_listening_pids
set "%~1="
set "_TMP_PIDS="

for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; $p=Get-NetTCPConnection -State Listen -LocalPort %PORT% | Select-Object -ExpandProperty OwningProcess -Unique; if($p){($p | Sort-Object -Unique) -join ' '}"`) do (
    set "_TMP_PIDS=%%P"
)

if not defined _TMP_PIDS (
    for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; $pids=@(); netstat -ano -p tcp ^| Select-String 'LISTENING' ^| ForEach-Object { $parts = (($_.Line -split '\s+') ^| Where-Object { $_ }); if($parts.Count -ge 5 -and $parts[1] -match ':%PORT%$'){ $pids += $parts[-1] } }; $u=$pids ^| Sort-Object -Unique; if($u){$u -join ' '}"`) do (
        set "_TMP_PIDS=%%P"
    )
)

set "%~1=%_TMP_PIDS%"
exit /b 0

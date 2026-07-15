@echo off
setlocal enabledelayedexpansion

:: ==============================================================================
:: CONFIGURATION
:: ==============================================================================
:: set "SERVER_IP=192.168.1.5"
set "SERVER_IP=127.0.0.1"
set "SERVER_PORT=1581"
set "LOOP_COUNT=1"
set "DELAY_MS=500"

:: ==============================================================================
:: TRANSMISSION AND PAYLOAD GENERATION
:: ==============================================================================
echo Starting transmission of %LOOP_COUNT% packets to %SERVER_IP%:%SERVER_PORT%...
set /a "wait_seconds=%DELAY_MS% / 1000"
echo Waiting for packet (%wait_seconds%s)... Otherwise, this process will terminate.
echo ----------------------------------------------------------------

for /L %%i in (1,1,%LOOP_COUNT%) do (

    :: PowerShell genera la sequenza da 255 a 0 in byte e la spara via UDP immediatamente
    powershell -Command ^
        "$bytes = [byte[]](255..0);" ^
        "$udpClient = New-Object System.Net.Sockets.UdpClient;" ^
        "$udpClient.Send($bytes, $bytes.Length, '%SERVER_IP%', %SERVER_PORT%);" ^
        "$udpClient.Close();"

    echo [Packet %%i/%LOOP_COUNT%] Sequence sent successfully.

    if %%i LSS %LOOP_COUNT% (
        powershell -Command "Start-Sleep -Milliseconds %DELAY_MS%"
    )
)

echo ----------------------------------------------------------------
echo Transmission completed. All %LOOP_COUNT% packets have been sent.
endlocal
pause

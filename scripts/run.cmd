@echo off

echo.
echo Start Running ...
echo.

:: Include the common environment file for paths
call "%~dp0env.cmd"

:: Save current directory to return later
set "current_path=%cd%"

echo.
if not exist "%tbt_bin_path%" (
    echo  ERROR! : folder "%tbt_bin_path%" does not exist, run ".\scripts\build.cmd" before
    echo.
    exit /b 1
)

:: Move into bin path and execute all binaries found
cd /d "%tbt_bin_path%"
for /f "tokens=*" %%f in ('dir /b *.exe') do (
    echo run executable file : %%f
    "%%f"
    if errorlevel 1 (
        echo.
        echo AARRGGHH!!! %%f executable failed. Exiting script.
        echo.
        cd /d "%current_path%"
        exit /b 1
    )
)
cd /d "%current_path%"

echo.
echo ... done!
echo.
exit /b 0
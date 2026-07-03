@echo off

echo.
echo Starting cleanup ...
echo.

:: Include the common environment file for paths
call "%~dp0env.cmd"

:: Remove build directory if it exists
if exist "%tbt_build_path%" (
    echo remove directory: %tbt_build_path%
    rmdir /s /q "%tbt_build_path%"
)

:: Remove bin directory if it exists
if exist "%tbt_bin_path%" (
    echo remove directory: %tbt_bin_path%
    rmdir /s /q "%tbt_bin_path%"
)

:: Remove deps directory if it exists
if exist "%tbt_deps_path%" (
    echo remove directory: %tbt_deps_path%
    rmdir /s /q "%tbt_deps_path%"
)

:: Remove log directory if it exists
if exist "%tbt_log_path%" (
    echo remove directory: %tbt_log_path%
    rmdir /s /q "%tbt_log_path%"
)

:: Delete files ending with a tilde (e.g., file.txt~) across all subdirectories
del /s /f /q /a "%tbt_root_path%\*~" 2>nul

:: Delete Vim swap files (.swp, .swo, .swn, etc.) across all subdirectories
del /s /f /q /a "%tbt_root_path%\*.sw?" 2>nul





echo.
echo ... done!
echo.
exit /b 0
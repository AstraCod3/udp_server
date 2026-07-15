@echo off
echo.
echo Starting cleanup ...
:: Include the common environment file for paths
call "%~dp0env.cmd"
:: Remove build directory if it exists
if exist "%build_path%" (
    echo remove directory: %build_path%
    rmdir /s /q "%build_path%"
)
:: Remove bin directory if it exists
if exist "%bin_path%" (
    echo remove directory: %bin_path%
    rmdir /s /q "%bin_path%"
)
:: Remove deps directory if it exists
if exist "%deps_path%" (
    echo remove directory: %deps_path%
    rmdir /s /q "%deps_path%"
)
:: Remove log directory if it exists
if exist "%log_path%" (
    echo remove directory: %log_path%
    rmdir /s /q "%log_path%"
)
:: Delete files ending with a tilde (e.g., file.txt~) across all subdirectories
del /s /f /q /a "%root_path%\*~" 2>nul
:: Delete Vim swap files (.swp, .swo, .swn, etc.) across all subdirectories
del /s /f /q /a "%root_path%\*.sw?" 2>nul
echo.
exit /b 0
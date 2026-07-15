@echo off
setlocal enabledelayedexpansion

echo.
echo Running Binary ...
echo.

call "%~dp0env.cmd"

set "current_path=%cd%"

if not exist "%bin_path%" (
    echo  ERROR! : folder "%bin_path%" does not exist. Run "build.cmd" before.
    echo.
    exit /b 1
)

cd /d "%bin_path%"

for %%f in (unit_test*.exe) do (
    if exist "%%f" (
    echo run executable file : %%f
    "%%f"
        if !errorlevel! neq 0 (
        echo.
        echo AARRGGHH!!! %%f executable failed. Exiting script.
        echo.
        cd /d "%current_path%"
        exit /b 1
    )
        echo.
    )
)

for %%f in (integration*.exe) do (
    if exist "%%f" (
        echo run executable file : %%f
        "%%f"
        if !errorlevel! neq 0 (
            echo.
            echo AARRGGHH!!! %%f executable failed. Exiting script.
            echo.
            cd /d "%current_path%"
            exit /b 1
        )
        echo.
    )
)

for %%f in (example*.exe) do (
    if exist "%%f" (
        echo run executable file : %%f
        "%%f"
        if !errorlevel! neq 0 (
            echo.
            echo AARRGGHH!!! %%f executable failed. Exiting script.
            echo.
            cd /d "%current_path%"
            exit /b 1
        )
        echo.
    )
)

cd /d "%current_path%"

exit /b 0

@echo off
setlocal enabledelayedexpansion

echo.
echo Starting Build ...
echo.

:: Include the common environment file for paths
call "%~dp0env.cmd"

:: Set default actions and build type
set "build_type=Release"
set "build_tests=on"
set "build_examples=on"

:: If no arguments provided, default to building the project
:parse_args
if "%~1"=="" goto end_parse_args

if "%~1"=="-h" goto show_help
if "%~1"=="--help" goto show_help
if "%~1"=="debug" (
    set "build_type=Debug"
    shift
    goto parse_args
)
if "%~1"=="release" (
    set "build_type=Release"
    shift
    goto parse_args
)
if "%~1"=="tests" (
    set "build_examples=off"
    set "build_tests=on"
    shift
    goto parse_args
)
if "%~1"=="examples" (
    set "build_examples=on"
    set "build_tests=off"
    shift
    goto parse_args
)

:: If argument is invalid, show error
echo Error: Invalid option '%~1'
echo Use -h or --help to see available options.
exit /b 1

:end_parse_args

set "current_path=%cd%"

if "%build_tests%"=="on" (
    if not exist "%build_tests_path%" mkdir "%build_tests_path%"
    cd /d "%build_tests_path%"

    if not exist "%deps_path%" (
        echo GoogleTest Framework Download and Build
        git clone https://github.com/google/googletest.git "%deps_path%/googletest"
        cmake -S "%deps_path%/googletest" -B "%deps_path%/googletest/build" -DCMAKE_INSTALL_PREFIX="%deps_path%/out" -DINSTALL_GTEST=ON
        cmake --build "%deps_path%/googletest/build" --config %build_type% --target install
    ) else (
        echo GoogleTest Framework already exists
    )

    echo.
    echo Building Unit and Integration Tests
    cmake "%tests_path%" -DCMAKE_BUILD_TYPE=%build_type%
    cmake --build . --config %build_type%
)

if "%build_examples%"=="on" (
    if not exist "%build_examples_path%" mkdir "%build_examples_path%"
    cd /d "%build_examples_path%"
    echo.
    echo Building examples
    cmake "%examples_path%" -DCMAKE_BUILD_TYPE=%build_type%
    cmake --build . --config %build_type%
)

cd /d "%current_path%"

echo Cleaning debug symbols ...
if exist "%bin_path%\*.pdb" del /f /q "%bin_path%\*.pdb"
if exist "%bin_path%\*.ilk" del /f /q "%bin_path%\*.ilk"
if exist "%bin_path%\*.exp" del /f /q "%bin_path%\*.exp"
if exist "%bin_path%\*.lib" del /f /q "%bin_path%\*.lib"

exit /b 0

:show_help
echo Usage: %~nx0 [OPTION]
echo.
echo Available options:
echo   -h, --help     Print this menu and exit
echo   debug          Compile the project in Debug mode (with symbols)
echo   release        Compile the project in Release mode (optimized)
echo   tests          Compile only the tests (default true)
echo   examples       Compile only the examples (default true)
echo.
echo  Compile examples in release mode
echo Example: %~nx0 example release
echo.
echo  Compile tests in debug release mode
echo Example: %~nx0 debug tests
exit /b 0
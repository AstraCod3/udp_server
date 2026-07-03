@echo off

:: Include the common environment file for paths
call "%~dp0env.cmd"

:: Set default actions and build type
set "build_type=Release"
set "run_build="
set "run_doc="

:: If no arguments provided, default to building the project
if "%~1"=="" (
    set "run_build=1"
    goto execute_actions
)

:parse_arguments
if "%~1" == "" goto execute_actions

if "%~1" == "-h" goto show_help
if "%~1" == "--help" goto show_help

if "%~1" == "debug" (
    set "build_type=Debug"
    set "run_build=1"
    shift
    goto parse_arguments
)
if "%~1" == "release" (
    set "build_type=Release"
    set "run_build=1"
    shift
    goto parse_arguments
)
if "%~1" == "doc" (
    set "run_doc=1"
    shift
    goto parse_arguments
)
if "%~1" == "all" (
    set "build_type=Release"
    set "run_build=1"
    set "run_doc=1"
    shift
    goto parse_arguments
)

:: If argument is invalid, show error
echo Error: Invalid option '%~1'
echo Use -h or --help to see available options.
exit /b 1

:execute_actions

:: Save current directory to return later
set "current_path=%cd%"

if defined run_build (
    echo.
    echo Starting build ...
    echo.

    :: Check if dependencies directory exists
    if not exist "%tbt_deps_path%" (
        echo GoogleTest Download and Build
        git clone https://github.com/google/googletest.git "%tbt_deps_path%\googletest"
        
        :: Configure GoogleTest
        cmake -S "%tbt_deps_path%\googletest" -B "%tbt_deps_path%\googletest\build" -DCMAKE_INSTALL_PREFIX="%tbt_deps_path%\out" -DINSTALL_GTEST=ON
        
        :: Build and Install GoogleTest
        cmake --build "%tbt_deps_path%\googletest\build" --config %build_type% --target install
        echo.
    ) else (
        echo GoogleTest already exists
    )

    echo.
    :: Create build folder if it doesn't exist
    if not exist "%tbt_build_path%" mkdir "%tbt_build_path%"
    cd /d "%tbt_build_path%"

    echo.
    echo Test Build
    :: Configure your main project
    cmake "%tbt_test_path%" -DCMAKE_BUILD_TYPE="%build_type%"
    :: Build using CMake
    cmake --build . --config %build_type%

    echo.
    :: Create bin folder if it doesn't exist
    if not exist "%tbt_bin_path%" mkdir "%tbt_bin_path%"

    :: Copy the compiled binary safely (Fix drive/path syntax)
    copy /y "%tbt_build_path%\%build_type%\*.exe" "%tbt_bin_path%\" >nul 2>&1

    :: Return to the original directory
    cd /d "%current_path%"

    echo.
    echo ... build done!
    echo.
)

:: 2. Run the Doxygen generation if requested
:: 2. Run the Doxygen generation if requested
if defined run_doc (
    echo.
    echo Checking Dependencies and Generating Doxygen Docs

    REM Move up to the repository root directory (one level above scripts/)
    cd /d "%~dp0\.."

    REM Check if Doxygen is installed locally
    where doxygen >nul 2>nul
    if %errorlevel% equ 0 (
        echo Local Doxygen detected. Generating HTML documentation...
        doxygen Doxyfile
        goto check_latex
    )

    REM Fallback to Docker No-Install mode
    echo [WARNING] Doxygen is not installed locally.
    echo Attempting to use Docker to avoid local installations...
    where docker >nul 2>nul
    if %errorlevel% equ 0 (
        echo Launching temporary Doxygen and LaTeX container...
        docker run --rm -v "%cd%:/data" hachque/doxygen doxygen Doxyfile
        echo [OK] Documentation HTML and PDF generated via Docker inside doc_output/
        goto end_process
    )

    REM Error message if both are missing
    echo [ERROR] Neither Doxygen nor Docker were found on your Windows system.
    echo To generate documentation, you can either:
    echo  - Install Doxygen from the terminal: winget install DimitriVanHeesch.Doxygen
    echo  - Or launch Docker Desktop if already installed.
    cd /d "%current_path%"
    exit /b 1

    :check_latex
    where pdflatex >nul 2>nul
    if %errorlevel% equ 0 (
        if exist "doc_output\latex" (
            echo LaTeX detected. Compiling PDF manual...
            cd doc_output\latex
            make >nul 2>nul
            copy refman.pdf ..\ >nul 2>nul
            cd ..\..
            echo [OK] PDF manual generated at doc_output/refman.pdf
        )
    ) else (
        echo [INFO] Skipping PDF generation. HTML generated successfully.
    )
    
    cd /d "%current_path%"
    echo.
    echo ...done!
    echo.
)


:end_process
exit /b 0

:show_help
echo.
echo Help menu
echo.
echo Available options:
echo   debug          Compile the project in Debug mode (with symbols)
echo   release        Compile the project in Release mode (optimized)
echo   doc            Verify dependencies and generate Doxygen documentation (HTML/PDF)
echo.
echo   -h, --help     Print this menu and exit
echo.
echo Example: .\scripts\build.cmd release doc
echo Example: .\scripts\build.cmd debug doc
echo Example: .\scripts\build.cmd doc
echo.
echo.
exit /b 0

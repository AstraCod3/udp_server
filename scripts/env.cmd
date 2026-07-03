@echo off

:: 1. Find the absolute directory of the script and its parent (Root)
set "tbt_script_path=%~dp0"
:: Remove trailing slash if present
if "%tbt_script_path:~-1%"=="\" set "tbt_script_path=%tbt_script_path:~0,-1%"

:: 2. Set lowercase paths relative to project root
set "tbt_root_path=%tbt_script_path%\.."
set "tbt_build_path=%tbt_root_path%\build"
set "tbt_bin_path=%tbt_root_path%\bin"
set "tbt_deps_path=%tbt_root_path%\deps"
set "tbt_log_path=%tbt_root_path%\log"
set "tbt_test_path=%tbt_root_path%\test"

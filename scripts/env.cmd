@echo off
:: 1. Find the absolute directory of the script and its parent (Root)
set "script_path=%~dp0"
:: Remove trailing slash if present
if "%script_path:~-1%"=="\" set "script_path=%script_path:~0,-1%"
:: 2. Set lowercase paths relative to project root
set "root_path=%script_path%\.."
set "bin_path=%root_path%\bin"
set "build_path=%root_path%\build"
set "build_examples_path=%build_path%\examples"
set "build_tests_path=%build_path%\tests"
set "deps_path=%root_path%\deps"
set "examples_path=%root_path%\examples"
set "log_path=%root_path%\log"
set "tests_path=%root_path%\tests"
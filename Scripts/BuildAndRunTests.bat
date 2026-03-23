@echo off
setlocal enabledelayedexpansion

REM Get the project root directory (parent of Scripts folder)
set PROJECT_ROOT=%~dp0..
cd /d "%PROJECT_ROOT%"

echo Project root: %PROJECT_ROOT%
echo.

REM Configure CMake with tests enabled
echo Configuring CMake...
cmake -B build -DRATUI_BUILD_TESTS=ON
if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build the project
echo.
echo Building project...
cmake --build build
if errorlevel 1 (
    echo Build failed!      
    exit /b 1
)

REM Run the tests in temporary directory
echo.
echo Running tests...
ctest --test-dir "%PROJECT_ROOT%\build" --output-on-failure
if errorlevel 1 (
    echo Tests failed!
    cd /d "%PROJECT_ROOT%"
    exit /b 1
)

cd /d "%PROJECT_ROOT%"
echo.
echo All tests passed!
pause
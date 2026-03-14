@echo off
REM Minuan Browser - Windows Build Script

echo.
echo ====================================
echo   Minuan Browser Build (Windows)
echo ====================================
echo.

REM Check if CMake is installed
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake is not installed or not in PATH
    echo Please install CMake from: https://cmake.org/download/
    pause
    exit /b 1
)

REM Create build directory
if not exist build (
    echo Creating build directory...
    mkdir build
)

cd build

REM Run CMake
echo Configuring with CMake...
cmake .. -G "Visual Studio 16 2019"
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

REM Build
echo Building Minuan Browser...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ====================================
echo   Build Complete!
echo ====================================
echo.
echo Executable: bin\minuan.exe
echo.
pause

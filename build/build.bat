@echo off
setlocal

echo.
echo ====================================
echo   Minuan Browser Build (Windows)
echo ====================================
echo.

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake was not found in PATH.
    exit /b 1
)

set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
set "BUILD_DIR=%ROOT%\out\build"

echo Project: %ROOT%
echo Build:   %BUILD_DIR%
echo.

cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo ERROR: CMake configuration failed.
    echo Check that Qt 6 with the WebEngine module is installed and CMAKE_PREFIX_PATH is set.
    exit /b 1
)

cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 (
    echo ERROR: Minuan build failed.
    exit /b 1
)

ctest --test-dir "%BUILD_DIR%" -C Release --output-on-failure
if errorlevel 1 (
    echo ERROR: Minuan tests failed.
    exit /b 1
)

set "APP=%BUILD_DIR%\bin\Minuan.exe"
if not exist "%APP%" (
    echo ERROR: Expected executable was not produced: %APP%
    exit /b 1
)

where windeployqt >nul 2>&1
if not errorlevel 1 (
    windeployqt --no-translations "%APP%"
)

echo.
echo ====================================
echo   Build Complete
echo ====================================
echo Executable: %APP%
echo.
endlocal

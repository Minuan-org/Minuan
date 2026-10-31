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
if errorlevel 1 (
    echo ERROR: windeployqt was not found. Add Qt's bin folder to PATH.
    exit /b 1
)

windeployqt --release --compiler-runtime "%APP%"
if errorlevel 1 (
    echo ERROR: Qt runtime deployment failed.
    exit /b 1
)

for %%I in ("%APP%") do set "APP_DIR=%%~dpI"
if not exist "%APP_DIR%QtWebEngineProcess.exe" (
    echo ERROR: Qt WebEngine process was not deployed beside Minuan.exe.
    exit /b 1
)
if not exist "%APP_DIR%resources\qtwebengine_resources.pak" (
    echo ERROR: Qt WebEngine Chromium resources were not deployed.
    exit /b 1
)

echo.
echo ====================================
echo   Build Complete
echo ====================================
echo Executable: %APP%
echo.
endlocal

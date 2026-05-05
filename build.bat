@echo off
setlocal

set "BUILD_DIR=%~dp0build"

where cmake.exe >nul 2>nul
if %errorlevel% neq 0 (
    echo [build] CMake not found. Install from https://cmake.org/download/ and re-run.
    exit /b 1
)

set "GEN_ARG="
where cl.exe >nul 2>nul
if %errorlevel% neq 0 (
    where g++.exe >nul 2>nul
    if %errorlevel%==0 (
        set "GEN_ARG=-G "MinGW Makefiles""
    )
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set "SRC_DIR=%~dp0"
if "%SRC_DIR:~-1%"=="\" set "SRC_DIR=%SRC_DIR:~0,-1%"

echo [build] configuring (%GEN_ARG%) ...
cmake -S "%SRC_DIR%" -B "%BUILD_DIR%" %GEN_ARG% -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [build] configure failed
    exit /b 2
)

echo [build] compiling ...
cmake --build "%BUILD_DIR%" --config Release --parallel
if %errorlevel% neq 0 (
    echo [build] compile failed
    exit /b 3
)

echo.
echo [build] OK -^> %~dp0LoliLoader.exe
exit /b 0

@echo off
setlocal EnableDelayedExpansion
title LoliLoader (universal mod injector)

set "HERE=%~dp0"
set "MODS_DIR=%HERE%mods"
set "AGENT_SRC=%HERE%ResolverAgent.jar"
set "AGENT_DEST=%USERPROFILE%\ResolverAgent.jar"
set "TARGET_DIR=%USERPROFILE%\AppData\Roaming\.loliland\game-resources\clients\hitech\main\mods"
set "LAUNCHER=%USERPROFILE%\Desktop\LoliLand.exe"

echo ====================================================
echo    LoliLoader - universal LoliLand mod injector
echo ====================================================
echo.

if not exist "%AGENT_SRC%" (
    echo [ERROR] ResolverAgent.jar not found next to this script:
    echo         %AGENT_SRC%
    echo.
    echo Build it from the project root with:  gradlew build
    echo Then copy build\libs\ResolverAgent-1.0.jar here, renamed to ResolverAgent.jar
    pause
    exit /b 1
)
if not exist "%MODS_DIR%" (
    mkdir "%MODS_DIR%"
)

copy /Y "%AGENT_SRC%" "%AGENT_DEST%" >nul
if errorlevel 1 (
    echo [ERROR] Could not copy agent to %AGENT_DEST%
    pause
    exit /b 1
)

set "JAR_COUNT=0"
for %%F in ("%MODS_DIR%\*.jar") do set /a JAR_COUNT+=1
echo [+] mods folder: %MODS_DIR%
echo [+] target folder: %TARGET_DIR%
echo [+] discovered %JAR_COUNT% mod jar(s):
for %%F in ("%MODS_DIR%\*.jar") do echo     - %%~nxF
if %JAR_COUNT%==0 (
    echo [!] No .jar files in mods folder. Drop your jars there and re-run.
    echo [!] Continuing anyway in case you only want the agent loaded.
)
echo.

set "JAVA_TOOL_OPTIONS=-javaagent:%AGENT_DEST%"
echo [+] JAVA_TOOL_OPTIONS = %JAVA_TOOL_OPTIONS%

if exist "%LAUNCHER%" (
    echo [+] launching %LAUNCHER%
    start "" "%LAUNCHER%"
) else (
    echo [!] %LAUNCHER% not found. Launch LoliLand manually now.
    echo     Press any key once the launcher window is visible.
    pause >nul
)

echo.
echo [!] Waiting for the game JVM (after you click Play in the launcher)...
echo     Press Ctrl+C to abort.
echo.

powershell -NoProfile -Command ^
    "$src='%MODS_DIR%'; $tgt='%TARGET_DIR%';" ^
    "Write-Host '[ps] scanning for game JVM...' -ForegroundColor Gray;" ^
    "while($true) {" ^
    "    $p = Get-CimInstance Win32_Process -Filter \"name = 'javaw.exe'\" | Where-Object { $_.CommandLine -like '*loliland.client.systemName*' };" ^
    "    if ($p) {" ^
    "        Write-Host '[ps] game detected. Hammering mods...' -ForegroundColor Green;" ^
    "        if (!(Test-Path $tgt)) { New-Item -ItemType Directory -Path $tgt -Force | Out-Null };" ^
    "        $jars = Get-ChildItem -Path $src -Filter '*.jar' -File -ErrorAction SilentlyContinue;" ^
    "        Write-Host ('[ps] mods to inject: ' + $jars.Count) -ForegroundColor Cyan;" ^
    "        while($true) {" ^
    "            $still = Get-CimInstance Win32_Process -Filter \"name = 'javaw.exe'\" | Where-Object { $_.CommandLine -like '*loliland.client.systemName*' };" ^
    "            if (!$still) { break };" ^
    "            foreach ($jar in $jars) {" ^
    "                $dst = Join-Path $tgt $jar.Name;" ^
    "                if (!(Test-Path $dst)) {" ^
    "                    Copy-Item -LiteralPath $jar.FullName -Destination $dst -Force -ErrorAction SilentlyContinue;" ^
    "                    if (Test-Path $dst) { attrib +h $dst 2>$null };" ^
    "                };" ^
    "            };" ^
    "            Start-Sleep -Milliseconds 20;" ^
    "        };" ^
    "        Write-Host '[ps] game closed. Cleaning up...' -ForegroundColor Yellow;" ^
    "        foreach ($jar in $jars) {" ^
    "            $dst = Join-Path $tgt $jar.Name;" ^
    "            Remove-Item -LiteralPath $dst -Force -ErrorAction SilentlyContinue;" ^
    "        };" ^
    "        Write-Host '[ps] done.' -ForegroundColor Cyan;" ^
    "        exit;" ^
    "    };" ^
    "    Start-Sleep -Milliseconds 200;" ^
    "}"

pause

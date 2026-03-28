@echo off
setlocal EnableExtensions
title Install MSVC Build Tools + MFC (for CouplingAssembly1)

net session >nul 2>&1
if %errorlevel% neq 0 (
  echo Requesting Administrator rights...
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b 0
)

cd /d "%~dp0"

set "SETUP=%TEMP%\vs_BuildTools.exe"
set "LOG=%TEMP%\vs_buildtools_coupling_install.log"

echo.
echo === Visual Studio Build Tools 2022: C++ + MFC ===
echo This downloads the official Microsoft installer (~3-4 MB bootstrap),
echo then installs components (several GB). Progress window will appear.
echo Log: %LOG%
echo.

where curl >nul 2>&1
if %errorlevel% equ 0 (
  curl -fL --retry 3 --retry-delay 2 -o "%SETUP%" "https://aka.ms/vs/17/release/vs_BuildTools.exe"
) else (
  powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "try { Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile '%SETUP%' -UseBasicParsing } catch { exit 1 }"
)

if not exist "%SETUP%" (
  echo [ERROR] Failed to download vs_BuildTools.exe
  pause
  exit /b 1
)

echo Starting installer (passive UI, please wait)...
"%SETUP%" --passive --wait --norestart ^
  --add Microsoft.VisualStudio.Workload.VCTools ^
  --add Microsoft.VisualStudio.Component.VC.ATLMFC ^
  --includeRecommended ^
  --log "%LOG%"

set "ERR=%errorlevel%"
echo.
if %ERR% equ 0 (
  echo [OK] Installation finished. Restart PC if Windows asks.
  echo Then run: msbuild_build.bat
) else (
  echo [ERROR] Installer exit code: %ERR%
  echo Open log: %LOG%
  echo Or install manually: https://visualstudio.microsoft.com/downloads/
)
echo.
pause
exit /b %ERR%

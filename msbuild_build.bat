@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "CFG=%~1"
set "PLAT=%~2"
if "%CFG%"=="" set "CFG=Debug"
if "%PLAT%"=="" set "PLAT=x64"

set "SLN=%~dp0CouplingAssembly1.sln"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
  echo.
  echo [ERROR] vswhere.exe not found.
  echo Install Visual Studio 2022 OR "Build Tools for Visual Studio 2022"
  echo with workload: "Desktop development with C++" and component "MFC".
  echo.
  exit /b 1
)

set "MSBUILD="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"

if not defined MSBUILD (
  echo [ERROR] MSBuild.exe not found. Install MSVC build tools.
  exit /b 1
)

echo MSBuild: !MSBUILD!
echo Solution: %SLN%
echo Config: %CFG%  Platform: %PLAT%
echo.

"%MSBUILD%" "%SLN%" /t:Build /p:Configuration=%CFG% /p:Platform=%PLAT% /m
if errorlevel 1 exit /b 1

if /i "%PLAT%"=="x64" (
  set "EXE=%~dp0CouplingAssembly1\x64\%CFG%\CouplingAssembly1.exe"
) else (
  set "EXE=%~dp0CouplingAssembly1\%CFG%\CouplingAssembly1.exe"
)

echo.
if exist "!EXE!" (
  echo OK: !EXE!
) else (
  echo Build reported success but EXE not found:
  echo   !EXE!
)
exit /b 0

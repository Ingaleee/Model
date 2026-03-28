@echo off
cd /d "%~dp0"
call "%~dp0msbuild_build.bat" Debug x64
if errorlevel 1 exit /b 1
set "EXE=%~dp0CouplingAssembly1\x64\Debug\CouplingAssembly1.exe"
if not exist "%EXE%" (
  echo EXE missing: %EXE%
  exit /b 1
)
start "" "%EXE%"

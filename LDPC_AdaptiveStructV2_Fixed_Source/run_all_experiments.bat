@echo off
set FRAMES=%1
set SEED=%2
if "%FRAMES%"=="" set FRAMES=20000
if "%SEED%"=="" set SEED=20260428
powershell -ExecutionPolicy Bypass -File "%~dp0run_all_experiments.ps1" -Frames %FRAMES% -Seed %SEED%
pause

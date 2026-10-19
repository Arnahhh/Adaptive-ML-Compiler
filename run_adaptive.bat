@echo off
setlocal
cd /d "%~dp0"
if "%~1"=="" (
    py adaptive_cli.py
) else (
    py adaptive_cli.py "%~1"
)

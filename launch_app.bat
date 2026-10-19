@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>nul
if errorlevel 1 (
  echo Python launcher ^(py^) was not found. Install Python 3.10+ and make sure it is on PATH.
  pause
  exit /b 1
)
py -c "import tkinter" >nul 2>nul
if errorlevel 1 (
  echo Tkinter is missing from this Python installation.
  echo Reinstall Python with Tcl/Tk support enabled, then retry.
  pause
  exit /b 1
)
py .\adaptive_app.py
if errorlevel 1 pause

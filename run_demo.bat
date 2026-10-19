@echo off
cd /d "%~dp0"
if not exist amlc.exe call build.bat
echo.
echo ===== DEMO 1: C PROGRAM =====
amlc.exe examples/demo.c
echo.
echo ===== DEMO 2: JAVA PROGRAM =====
amlc.exe examples/java_basic.java
echo.
echo ===== DEMO 3: PROGRAM REPAIR =====
amlc.exe examples/repair_missing_semicolon.java
pause

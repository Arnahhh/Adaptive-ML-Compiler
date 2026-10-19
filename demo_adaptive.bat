@echo off
cd /d "%~dp0"
if not exist amlc.exe call build.bat

echo.
echo ==================================================
echo DEMO 1 - CONSTANT / ARITHMETIC PROGRAM
echo ==================================================

echo.
amlc.exe examples\adaptive_constant.c

 echo.
echo ==================================================
echo DEMO 2 - LOOP-HEAVY PROGRAM
echo ==================================================

echo.
amlc.exe examples\adaptive_loop.c

 echo.
echo ==================================================
echo DEMO 3 - BRANCH-HEAVY PROGRAM
echo ==================================================

echo.
amlc.exe examples\adaptive_branch.c

pause

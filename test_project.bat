@echo off
setlocal
cd /d "%~dp0"
py -m unittest -v
set RESULT=%ERRORLEVEL%
echo.
if "%RESULT%"=="0" (echo All available project tests passed.) else (echo One or more project tests failed.)
pause
exit /b %RESULT%

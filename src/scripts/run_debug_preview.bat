@echo off

REM ### Run Friction debug build with preview logging enabled
REM # Captures output to friction-debug.log in the same directory
REM #
REM # Usage:
REM #   run_debug_preview.bat                (runs friction.exe in current directory)
REM #   run_debug_preview.bat "C:\path\to\friction.exe"
REM #
REM # After reproducing the blank preview issue, send friction-debug.log
REM # to the Friction bug tracker: https://github.com/friction2d/friction/issues

set EXE=%1
if "%EXE%"=="" set EXE=friction.exe

set QT_LOGGING_RULES=friction.renderhandler=true;friction.cachehandler=true;friction.canvas=true;friction.renderoutput=true;friction.audio=true
set LOG_FILE=%~dp0friction-debug.log

echo Friction debug preview log > "%LOG_FILE%"
echo Started: %DATE% %TIME% >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

echo Logging to %LOG_FILE%
echo Reproduce the blank preview issue, then close Friction.

"%EXE%" >> "%LOG_FILE%" 2>&1

echo.
echo Log saved to: %LOG_FILE%

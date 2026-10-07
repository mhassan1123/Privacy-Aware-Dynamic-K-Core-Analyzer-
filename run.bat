@echo off
cd /d "%~dp0"
if not exist server.exe (
  echo server.exe not found. Double-click build.bat first.
  pause
  exit /b
)
rem open the browser after 2 seconds, then start the server
start "" cmd /c "timeout /t 2 >nul & start http://localhost:8080"
server.exe
pause
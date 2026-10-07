@echo off
cd /d "%~dp0"
echo Compiling...
g++ -std=c++17 -DNOMINMAX -D_WIN32_WINNT=0x0A00 server.cpp DynamicGraph.cpp kcore.cpp graph.cpp PrivacyManager.cpp -o server.exe -lws2_32
if errorlevel 1 (
  echo.
  echo BUILD FAILED - read the error above.
) else (
  echo.
  echo BUILD OK - now double-click run.bat
)
pause
 
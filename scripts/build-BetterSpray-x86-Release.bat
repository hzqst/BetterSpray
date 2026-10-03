@echo off
setlocal
set "Configuration=Release"
call "%~dp0build-BetterSpray-x86.bat" %*
exit /b %errorlevel%

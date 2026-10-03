@echo off
setlocal
set "Configuration=Debug"
call "%~dp0build-BetterSpray-x86.bat" %*
exit /b %errorlevel%

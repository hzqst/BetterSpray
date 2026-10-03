@echo off
setlocal
if not "%Configuration%"=="Debug" if not "%Configuration%"=="Release" exit /b 2
if not defined SolutionDir set "SolutionDir=%~dp0.."
for %%I in ("%SolutionDir%") do set "SolutionDir=%%~fI"
if not "%SolutionDir:~-1%"=="\" set "SolutionDir=%SolutionDir%\"

cmake -G "Visual Studio 17 2022" -A Win32 -S "%SolutionDir%." -B "%SolutionDir%build\x86\%Configuration%" -DCMAKE_INSTALL_PREFIX="%SolutionDir%install\x86\%Configuration%" %*
if errorlevel 1 exit /b %errorlevel%
cmake --build "%SolutionDir%build\x86\%Configuration%" --config %Configuration% --target install --parallel
exit /b %errorlevel%

@echo off

setlocal

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

for /f "usebackq tokens=*" %%i in (`thirdparty\MetaHookSv\tools\vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

cd /d "%SolutionDir%thirdparty\libxml2-win-build\build-VS2022-MT"

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat"

    MSBuild.exe libxml2.sln "/target:libxml2" /p:Configuration="Debug" /p:Platform="Win32"
)

endlocal
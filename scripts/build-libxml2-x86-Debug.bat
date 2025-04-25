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

:: Check if libxml2-win-build directory has been initialized
if not exist "%SolutionDir%thirdparty\libxml2-win-build\.git" (
    echo Initializing libxml2-win-build submodule only...
    :: Initialize only the libxml2-win-build submodule without recursive initialization
    call git submodule update --init "%SolutionDir%thirdparty\libxml2-win-build"
    if errorlevel 1 (
        echo Error: git submodule initialization failed!
        exit /b 1
    )
    echo submodule initialization completed.
)

for /f "usebackq tokens=*" %%i in (`thirdparty\MetaHookSv\tools\vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat"

    cd /d "%SolutionDir%thirdparty\libxml2-win-build\build-VS2022-MT"

    MSBuild.exe libxml2.sln "/target:libxml2" /p:Configuration="Debug" /p:Platform="Win32"
)

endlocal
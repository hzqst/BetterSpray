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

:: Check if xz-win-build directory has been initialized
if not exist "%SolutionDir%thirdparty\xz-win-build\.git" (
    echo Initializing xz-win-build submodule only...
    :: Initialize only the xz-win-build submodule without recursive initialization
    call git submodule update --init "%SolutionDir%thirdparty\xz-win-build"
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

    cd /d "%SolutionDir%thirdparty\xz-win-build\build-VS2022-MT"

    MSBuild.exe xz.sln "/target:liblzma-static" /p:Configuration="Release" /p:Platform="Win32"
)

endlocal
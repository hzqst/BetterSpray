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

:: Check if curl directory has been initialized
if not exist "%SolutionDir%thirdparty\sqlite3-cmake\CMakeLists.txt" if not exist "%SolutionDir%thirdparty\sqlite3-cmake\.git" (
    echo Initializing sqlite3-cmake submodule only...
    :: Initialize only the sqlite3-cmake submodule without recursive initialization
    call git submodule update --init "%SolutionDir%thirdparty\sqlite3-cmake"
    if errorlevel 1 (
        echo Error: git submodule initialization failed!
        exit /b 1
    )
    echo submodule initialization completed.
)

call cmake -G "Visual Studio 17 2022" -S "%SolutionDir%thirdparty\sqlite3-cmake" -B "%SolutionDir%thirdparty\build\sqlite3\x86\Release" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\sqlite3\x86\Release" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%thirdparty\MetaHookSv\tools\toolchain.cmake" -Dsqlite3_BUILD_SHARED_LIBS=TRUE

call cmake --build "%SolutionDir%thirdparty\build\sqlite3\x86\Release" --config Release --target install

endlocal
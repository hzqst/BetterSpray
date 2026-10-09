# Shared implementation: explicit override, surrounding aggregator, pinned archive.
set(METAHOOKSV_LAUNCH_GAME_MODULE_DIR "" CACHE PATH "Directory containing the shared LaunchGame.cmake module")
if(METAHOOKSV_LAUNCH_GAME_MODULE_DIR)
    set(_launch_module_dir "${METAHOOKSV_LAUNCH_GAME_MODULE_DIR}")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../cmake/LaunchGame.cmake")
    get_filename_component(_launch_module_dir "${CMAKE_CURRENT_LIST_DIR}/../../../cmake" ABSOLUTE)
else()
    include(FetchContent)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(metahooksv_launch_module
        URL https://github.com/MetaHookSv/MetaHookSv/archive/launch-game-cmake-v3.tar.gz
        TLS_VERIFY ON
        # Populate only the modules; never configure the aggregate native project.
        SOURCE_SUBDIR cmake)
    FetchContent_MakeAvailable(metahooksv_launch_module)
    set(_launch_module_dir "${metahooksv_launch_module_SOURCE_DIR}/cmake")
endif()
if(NOT EXISTS "${_launch_module_dir}/LaunchGame.cmake")
    message(FATAL_ERROR "METAHOOKSV_LAUNCH_GAME_MODULE_DIR must contain LaunchGame.cmake: ${_launch_module_dir}")
endif()
include("${_launch_module_dir}/LaunchGame.cmake")
unset(_launch_module_dir)

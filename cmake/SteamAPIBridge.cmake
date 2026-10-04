# Standalone consumers build the same shared bridge as the aggregate.
set(STEAMAPIBRIDGE_SOURCE_PATH "$ENV{STEAMAPIBRIDGE_SOURCE_PATH}" CACHE PATH "SteamAPIBridge source tree; empty fetches the pinned commit")
if(NOT TARGET SteamAPIBridge)
    if(NOT STEAMAPIBRIDGE_SOURCE_PATH)
        include(FetchContent)
        FetchContent_Declare(betterspray_steambridge
            GIT_REPOSITORY https://github.com/MetaHookSv/SteamAPIBridge
            GIT_TAG 69d0d8c19dd03539264cf635980e362d0cfae536
            GIT_SUBMODULES "" SOURCE_SUBDIR _source_only)
        FetchContent_MakeAvailable(betterspray_steambridge)
        set(STEAMAPIBRIDGE_SOURCE_PATH "${betterspray_steambridge_SOURCE_DIR}")
    endif()
    if(NOT EXISTS "${STEAMAPIBRIDGE_SOURCE_PATH}/CMakeLists.txt")
        message(FATAL_ERROR "STEAMAPIBRIDGE_SOURCE_PATH must contain CMakeLists.txt")
    endif()
    add_subdirectory("${STEAMAPIBRIDGE_SOURCE_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/SteamAPIBridge")
endif()

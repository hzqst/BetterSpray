cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(BetterSpray LANGUAGES C CXX RC)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "BetterSpray requires Windows MSVC x86. Configure with -A Win32.")
endif()
get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "Select Debug or Release.")
endif()

include(cmake/Dependencies.cmake)
betterspray_prepare_dependencies()
include("${VC_LTL_Root}/VC-LTL helper for cmake.cmake")
include(cmake/ImageLibraries.cmake)
betterspray_add_image_libraries()
include(cmake/Sources.cmake)

add_library(betterspray_settings INTERFACE)
target_compile_features(betterspray_settings INTERFACE cxx_std_20)
target_include_directories(betterspray_settings INTERFACE
    "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/include/Interface"
    "${VGUI2EXTENSION_SOURCE_PATH}/include/Interface"
    "${VGUI2EXTENSION_SOURCE_PATH}/include/Interface/VGUI"
    "${UTILTHREADTASK_SOURCE_PATH}/include/Interface"
    "${UTILHTTPCLIENT_SOURCE_PATH}/include/Interface"
    "${METAHOOK_SOURCE_PATH}/include"
    "${METAHOOK_SOURCE_PATH}/include/Interface"
    "${METAHOOK_SOURCE_PATH}/include/Interface/VGUI"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/cl_dll"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/common"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/engine"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/pm_shared"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/public"
    "${METAHOOK_SOURCE_PATH}/include/VGUI"
    "${METAHOOK_SOURCE_PATH}/include/vgui_controls"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK/tier0"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK/tier1"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK/vstdlib"
    "${STEAMSDK_SOURCE_PATH}" "${SCOPEEXIT_SOURCE_PATH}/include" "${CHOCOBO1HASH_SOURCE_PATH}/src")
target_compile_definitions(betterspray_settings INTERFACE WIN32 _WINDOWS _MBCS
    _CRT_SECURE_NO_WARNINGS VGUI_USE_SURFACE2 VGUI_USE_SCHEME2 VGUI_USE_INPUT2
    NO_MALLOC_OVERRIDE NO_TIER0_MINIDUMP NO_VCR
    $<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Release>:NDEBUG>)
target_compile_options(betterspray_settings INTERFACE /permissive- /W3 /wd4819 /wd4996)

add_library(BetterSpray SHARED ${BETTERSPRAY_SOURCES})
target_link_libraries(BetterSpray PRIVATE betterspray_settings FreeImage LibXml2::LibXml2 SteamSDK::SteamAPI)
target_compile_definitions(BetterSpray PRIVATE BETTERSPRAY_EXPORTS _USRDLL)
target_compile_options(BetterSpray PRIVATE /MP /sdl /Zi $<$<CONFIG:Release>:/Gy;/Oi>)
target_link_options(BetterSpray PRIVATE /DEBUG /SUBSYSTEM:WINDOWS
    $<$<CONFIG:Debug>:/INCREMENTAL;/NODEFAULTLIB:MSVCRT>
    $<$<CONFIG:Release>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF>)
set_target_properties(BetterSpray PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

install(TARGETS BetterSpray RUNTIME DESTINATION svencoop/metahook/plugins)
install(FILES "$<TARGET_PDB_FILE:BetterSpray>" DESTINATION svencoop/metahook/plugins)
install(FILES "$<TARGET_FILE:FreeImage>" "$<TARGET_PDB_FILE:FreeImage>" DESTINATION svencoop/metahook/dlls/FreeImage)
install(FILES "$<TARGET_FILE:LibXml2>" "$<TARGET_PDB_FILE:LibXml2>" DESTINATION svencoop/metahook/dlls)
install(DIRECTORY assets/ DESTINATION .)
install(DIRECTORY include/ DESTINATION include)
set(license_dir svencoop/metahook/licenses/betterspray)
install(FILES "${FREEIMAGE_SOURCE_PATH}/license-fi.txt" "${FREEIMAGE_SOURCE_PATH}/license-gplv2.txt"
    "${FREEIMAGE_SOURCE_PATH}/license-gplv3.txt" DESTINATION ${license_dir}/FreeImage)
install(FILES "${LIBXML2_SOURCE_PATH}/Copyright" DESTINATION ${license_dir}/libxml2)
install(FILES "${SCOPEEXIT_SOURCE_PATH}/LICENSE" DESTINATION ${license_dir}/ScopeExit)
install(FILES "${CHOCOBO1HASH_SOURCE_PATH}/LICENSE" DESTINATION ${license_dir}/Chocobo1Hash)
install(FILES "${METAHOOK_SOURCE_PATH}/LICENSE" DESTINATION ${license_dir}/MetaHook)
install(FILES "${STEAMSDK_SOURCE_PATH}/STEAM-SDK-NOTICE.md" DESTINATION ${license_dir}/SteamSDK)
install(FILES "${VC_LTL_Root}/Readme.md" DESTINATION ${license_dir}/VC-LTL)

option(BETTERSPRAY_SYNC_GAMEDATA "Download and prune the plugin gamedata catalog before building" ON)
set(BETTERSPRAY_GAMEDATA_DIR "${PROJECT_BINARY_DIR}/assets/svencoop/metahook/gamedata/betterspray" CACHE PATH "Plugin gamedata catalog directory")
if(BETTERSPRAY_SYNC_GAMEDATA)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_target(BetterSprayGameData
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/sync-gamedata.py"
            --manifest "${PROJECT_SOURCE_DIR}/scripts/manifests/betterspray.json"
            --target-dir "${BETTERSPRAY_GAMEDATA_DIR}" --temp-root "${PROJECT_BINARY_DIR}/gamedata-sync"
        COMMENT "Synchronizing and validating BetterSpray gamedata" VERBATIM)
    add_custom_target(BetterSprayGameDataValidate
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/validate-gamedata.py"
            "${BETTERSPRAY_GAMEDATA_DIR}" --manifest "${PROJECT_SOURCE_DIR}/scripts/manifests/betterspray.json"
        COMMENT "Validating BetterSpray gamedata against the manifest" VERBATIM)
    add_dependencies(BetterSprayGameDataValidate BetterSprayGameData)
    add_dependencies(BetterSpray BetterSprayGameDataValidate)
endif()
install(DIRECTORY "${BETTERSPRAY_GAMEDATA_DIR}/" DESTINATION svencoop/metahook/gamedata/betterspray OPTIONAL)

option(BETTERSPRAY_BUILD_TESTS "Build gamedata, library and DLL loading regression tests" OFF)
if(BETTERSPRAY_BUILD_TESTS)
    enable_testing()
    include(cmake/Tests.cmake)
endif()

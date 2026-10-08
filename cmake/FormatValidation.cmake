# Shared tool implementation; only this dependency bootstrap belongs to the consumer.
if(NOT DEFINED FORMAT_VALIDATION_SOURCE_PATH)
    set(FORMAT_VALIDATION_SOURCE_PATH "$ENV{FORMAT_VALIDATION_SOURCE_PATH}" CACHE PATH
        "FormatValidation source tree; empty fetches the pinned tooling")
endif()
if(FORMAT_VALIDATION_SOURCE_PATH)
    get_filename_component(_format_validation_source "${FORMAT_VALIDATION_SOURCE_PATH}"
        ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
else()
    include(FetchContent)
    FetchContent_Declare(betterspray_format_validation
        GIT_REPOSITORY https://github.com/MetaHookSv/FormatValidation.git
        GIT_TAG 13c9fabe058e1f887ad1b03bb6884de911192c6a # v1.0.0; retained immutable tooling commit
        GIT_SUBMODULES ""
        # Populate the tooling without configuring its own test project.
        SOURCE_SUBDIR cmake)
    FetchContent_MakeAvailable(betterspray_format_validation)
    set(_format_validation_source "${betterspray_format_validation_SOURCE_DIR}")
endif()
foreach(_format_validation_file .clang-format clang-format-validate.py cmake/FormatValidation.cmake)
    if(NOT EXISTS "${_format_validation_source}/${_format_validation_file}")
        message(FATAL_ERROR "FORMAT_VALIDATION_SOURCE_PATH is missing ${_format_validation_file}: ${_format_validation_source}")
    endif()
endforeach()
include("${_format_validation_source}/cmake/FormatValidation.cmake")
format_validation_add_component(BetterSpray "${CMAKE_CURRENT_SOURCE_DIR}")

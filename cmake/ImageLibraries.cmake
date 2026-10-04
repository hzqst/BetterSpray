function(betterspray_add_image_libraries)
    # Directory/function scopes isolate vendor flags and options. The parent
    # already applies VC-LTL; dependency sources and install rules stay external.
    if(NOT TARGET FreeImage)
        set(USE_VCLTL OFF)
        add_subdirectory("${FREEIMAGE_SOURCE_PATH}" "${PROJECT_BINARY_DIR}/thirdparty/FreeImage" EXCLUDE_FROM_ALL)
    endif()
    # The install rules also need symbols when another component owns the target.
    target_compile_options(FreeImage PRIVATE /Zi)
    target_link_options(FreeImage PRIVATE /DEBUG)

    set(BUILD_SHARED_LIBS ON)
    foreach(option LIBXML2_WITH_HTTP LIBXML2_WITH_ICONV LIBXML2_WITH_ICU LIBXML2_WITH_LZMA
        LIBXML2_WITH_ZLIB LIBXML2_WITH_PROGRAMS LIBXML2_WITH_PYTHON LIBXML2_WITH_TESTS)
        set(${option} OFF)
    endforeach()
    set(LIBXML2_WITH_HTML ON)
    set(LIBXML2_WITH_XPATH ON)
    set(LIBXML2_WITH_THREADS ON)
    add_subdirectory("${LIBXML2_SOURCE_PATH}" "${PROJECT_BINARY_DIR}/thirdparty/libxml2" EXCLUDE_FROM_ALL)
    # Preserve the runtime filename used by the old MSBuild package.
    set_target_properties(LibXml2 PROPERTIES
        OUTPUT_NAME libxml2 PREFIX "" IMPORT_PREFIX "" DEBUG_POSTFIX "")
    target_compile_options(LibXml2 PRIVATE /Zi)
    target_link_options(LibXml2 PRIVATE /DEBUG)
endfunction()

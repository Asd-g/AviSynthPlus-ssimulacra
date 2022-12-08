cmake_minimum_required(VERSION 3.16)

project(jxlssimulacra LANGUAGES CXX)

###
add_library(ssimulacra_lib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/tools/ssimulacra.cc)

target_include_directories(ssimulacra_lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libjxl
    ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/include
)

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    endif()

    string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
    if (build_type STREQUAL debug)
        target_compile_definitions(ssimulacra_lib PRIVATE DEBUG_BUILD)
    else (build_type STREQUAL release)
        target_compile_definitions(ssimulacra_lib PRIVATE RELEASE_BUILD)
    endif()
endif()

target_compile_features(ssimulacra_lib PRIVATE cxx_std_17)

if (UNIX)
    target_compile_options(ssimulacra_lib PRIVATE $<$<CONFIG:RELEASE>:-ffunction-sections -s>)
endif()

find_library(libhwy NAMES hwy PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/lib)
message(STATUS "ssimulacra_lib: find_library returned ${libhwy}")


###
project(jxlssimulacra2 LANGUAGES CXX)

add_library(ssimulacra2_lib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/tools/ssimulacra2.cc)

target_include_directories(ssimulacra2_lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libjxl
    ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/include
)

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    endif()

    string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
    if (build_type STREQUAL debug)
        target_compile_definitions(ssimulacra2_lib PRIVATE DEBUG_BUILD)
    else (build_type STREQUAL release)
        target_compile_definitions(ssimulacra2_lib PRIVATE RELEASE_BUILD)
    endif()
endif()

target_compile_features(ssimulacra2_lib PRIVATE cxx_std_17)

if (UNIX)
    target_compile_options(ssimulacra2_lib PRIVATE $<$<CONFIG:RELEASE>:-ffunction-sections -s>)
endif()

find_library(libhwy NAMES hwy PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/lib)
message(STATUS "ssimulacra2_lib: find_library returned ${libhwy}")


###
project(libssimulacra LANGUAGES CXX)

set(sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/avs_ssimulacra.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/avs_ssimulacra2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.cpp
)

if (WIN32)
    set(sources ${sources} ${CMAKE_CURRENT_BINARY_DIR}/avs_ssimulacra.rc)
endif()

add_library(ssimulacra SHARED ${sources})

if (UNIX)
    target_include_directories(ssimulacra PRIVATE /usr/local/include/avisynth)
else()
    target_include_directories(ssimulacra PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../AviSynthPlus/avs_core/include")
endif()

target_include_directories(ssimulacra PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libjxl
    ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/include
)

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    endif()

    string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
    if (build_type STREQUAL debug)
        target_compile_definitions(ssimulacra PRIVATE DEBUG_BUILD)
    else (build_type STREQUAL release)
        target_compile_definitions(ssimulacra PRIVATE RELEASE_BUILD)
    endif()

    message(STATUS "Build type - ${CMAKE_BUILD_TYPE}")
endif()

target_compile_features(ssimulacra PRIVATE cxx_std_17)

if (UNIX)
    target_compile_options(ssimulacra PRIVATE $<$<CONFIG:RELEASE>:-ffunction-sections -s>)
endif()

if (NOT WIN32)
    find_library(libjxl NAMES jxl PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/lib)
    target_link_libraries(ssimulacra PRIVATE libavisynth.so)
else()
    find_library(libjxl NAMES jxl-static PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/lib)
    target_link_libraries(ssimulacra PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../AviSynthPlus/build_x64/Output/c_api/AviSynth.lib")
endif()
message(STATUS "avs_ssimulacra: find_library returned ${libjxl}")

find_library(libhwy NAMES hwy PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/lib)
message(STATUS "avs_ssimulacra: find_library returned ${libhwy}")

target_link_libraries(ssimulacra PRIVATE
    ${libjxl}
    ${libhwy}
)

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    target_link_libraries(ssimulacra PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/ssimulacra_lib.lib
        ${CMAKE_CURRENT_BINARY_DIR}/ssimulacra2_lib.lib
    )
else()
    target_link_libraries(ssimulacra PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/ssimulacra_lib.lib
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/ssimulacra2_lib.lib
    )
endif()

add_dependencies(ssimulacra ssimulacra_lib ssimulacra2_lib)

if (CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
    target_link_libraries(ssimulacra PRIVATE
        libmmds
        svml_disp
    )
endif()

find_package (Git)

if (GIT_FOUND)
    execute_process (COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        OUTPUT_VARIABLE ver
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    message (STATUS "GIT not found")
endif()

if (WIN32)
    configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/avs_ssimulacra.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/avs_ssimulacra.rc"
    )

    set_target_properties(ssimulacra PROPERTIES
        PREFIX ""
        OUTPUT_NAME "ssimulacra"
    )
else()
    set_target_properties(ssimulacra PROPERTIES OUTPUT_NAME "ssimulacra.${ver}")
endif()

if (UNIX)
    include(GNUInstallDirs)

    INSTALL(TARGETS ssimulacra LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")

    # uninstall target
    if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    endif()
endif()

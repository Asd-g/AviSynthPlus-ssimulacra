cmake_minimum_required(VERSION 3.16)

project(libssimulacra LANGUAGES CXX)

set(sources ${CMAKE_CURRENT_SOURCE_DIR}/src/avs_ssimulacra.cpp)

if (WIN32)
    set(sources ${sources} ${CMAKE_CURRENT_BINARY_DIR}/avs_ssimulacra.rc)
endif()

add_library(ssimulacra SHARED
    ${sources}
    ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/tools/ssimulacra.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/tools/ssimulacra2.cc
)

if (UNIX)
    target_include_directories(ssimulacra PRIVATE /usr/local/include/avisynth)
else()
    target_include_directories(ssimulacra PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../AviSynthPlus/avs_core/include")
endif()

target_include_directories(ssimulacra PRIVATE
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

find_library(libjxl
    NAMES jxl jxl-static
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/lib
)
find_library(libavs NAMES avisynth PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../AviSynthPlus/build_x64_clang/Output/c_api")
find_library(libhwy NAMES hwy PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libjxl/jxl_install/lib NO_DEFAULT_PATH)

message(STATUS "avs_ssimulacra: find_library returned ${libjxl}")
message(STATUS "avs_ssimulacra: find_library returned ${libavs}")
message(STATUS "avs_ssimulacra: find_library returned ${libhwy}")

target_link_libraries(ssimulacra PRIVATE
    ${libjxl}
    ${libavs}
    ${libhwy}
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
    target_link_libraries(ssimulacra PRIVATE
        libmmds
        svml_disp
    )
endif()

if (MINGW)
    target_link_libraries(ssimulacra PRIVATE -static-libstdc++ -static-libgcc -s)
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

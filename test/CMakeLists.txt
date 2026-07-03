cmake_minimum_required(VERSION 3.28.3)

set(PROJ_UDP_SERVER "udp_server")


# Global settings for Google Test (Valid for all platforms)
# Force gtest to use the dynamic runtime on Windows; has no negative side effects on Linux
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

project(${PROJ_UDP_SERVER}
  VERSION 0.4.0
  DESCRIPTION "project base of thread"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Compilatori basati su UNIX (GCC e Clang)
#
if (UNIX)
    message(STATUS "${PROJ_UDP_SERVER} :: OS Linux/Unix")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(STATUS "${PROJ_UDP_SERVER} :: Compile GNU")
        add_compile_options(
            -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion 
            -Wfloat-equal -Wdeprecated-declarations -Wnull-dereference 
            -Wformat -Wlogical-op -Wcast-align -Wstrict-aliasing 
            -Wmissing-declarations -Wmissing-include-dirs -Wfloat-conversion 
            -Wduplicated-branches -Wformat-security -Wcast-qual -Wnon-virtual-dtor
        )

        # Gestione corretta per generatori a configurazione singola (Make/Ninja)
        string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
        message(STATUS "${PROJ_UDP_SERVER} :: ${CMAKE_BUILD_TYPE} mode")

        #
        # DEBUG (G++)
        #
        if(BUILD_TOLOWER STREQUAL "debug")
            add_compile_options(-O0 -g3 -ggdb3)
        endif()

        #
        # RELEASE (G++)
        #
        if(BUILD_TOLOWER STREQUAL "release")
            add_compile_options(-O3)
            add_compile_definitions(NDEBUG)
        endif()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "${PROJ_UDP_SERVER} :: Compile Clang")
        message(STATUS "${PROJ_UDP_SERVER} :: ${CMAKE_BUILD_TYPE} mode")
        add_compile_options(-Wshadow)
    endif()
endif()

#
# Windows OS (MSVC)
#
if (WIN32)
    message(STATUS "${PROJ_UDP_SERVER} :: OS Win32")
    if (MSVC)
        message(STATUS "${PROJ_UDP_SERVER} :: microsoft visual studio compiler")
        message(STATUS "${PROJ_UDP_SERVER} :: ${CMAKE_BUILD_TYPE} mode")

        # Automatically sync MSVC runtime library with the build type (MDd for Debug / MD for Release)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

        add_compile_options(/W4 /WX /wd4068 /wd4189 /wd4706)
        #
        # DEBUG (MSVC)
        #
        add_compile_options(
            $<$<CONFIG:Debug>:/Od>  # Disable optimization debugging
            $<$<CONFIG:Debug>:/Zi>  # Generate debug info (.pdb)
            $<$<CONFIG:Debug>:/RTC1> # Enable runtime check Debug mode
        )
        #
        # RELEASE (MSVC)
        #
        add_compile_options(
            $<$<CONFIG:Release>:/O2> # Optimization max speed of code
            $<$<CONFIG:Release>:/Oi> # Enable functions
        )
        add_compile_definitions(
            $<$<CONFIG:Release>:NDEBUG> # Disable macro assert()
        )
    endif (MSVC)
endif (WIN32)

file( GLOB UDP_SERVER_SRC "*.cpp" )

set( UDP_SERVER_BIN test_udp_server )

add_executable( ${UDP_SERVER_BIN} ${UDP_SERVER_SRC} )

set( GTEST_INC_DIR "$ENV{deps_path}/out/include" )

target_include_directories(${UDP_SERVER_BIN} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GTEST_INC_DIR}
)

set( GTEST_LIB_DIR "$ENV{deps_path}/out/lib" )

find_library(GTEST_LIB_FILE 
    NAMES gtest gtestd 
    PATHS "${GTEST_LIB_DIR}" 
    NO_DEFAULT_PATH
)

find_library(GTEST_MAIN_FILE 
    NAMES gtest_main gtest_maind 
    PATHS "${GTEST_LIB_DIR}" 
    NO_DEFAULT_PATH
)

target_link_libraries(${UDP_SERVER_BIN} PRIVATE
    ${GTEST_LIB_FILE}
    ${GTEST_MAIN_FILE}
)


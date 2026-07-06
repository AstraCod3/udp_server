cmake_minimum_required(VERSION 3.28.3)

set(TESTS_UDP_SERVER "tests_udp_server")


# Global settings for Google Test (Valid for all platforms)
# Force gtest to use the dynamic runtime on Windows; has no negative side effects on Linux
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

project(${TESTS_UDP_SERVER}
  VERSION 0.4.0
  DESCRIPTION "project unit and integration tests of udp server"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Compilatori basati su UNIX (GCC e Clang)
#
if (UNIX)
    message(STATUS "${TESTS_UDP_SERVER} :: OS Linux/Unix")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(STATUS "${TESTS_UDP_SERVER} :: Compile GNU")
        add_compile_options(
            -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion 
            -Wfloat-equal -Wdeprecated-declarations -Wnull-dereference 
            -Wformat -Wlogical-op -Wcast-align -Wstrict-aliasing 
            -Wmissing-declarations -Wmissing-include-dirs -Wfloat-conversion 
            -Wduplicated-branches -Wformat-security -Wcast-qual -Wnon-virtual-dtor
        )

        # Gestione corretta per generatori a configurazione singola (Make/Ninja)
        string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
        message(STATUS "${TESTS_UDP_SERVER} :: ${CMAKE_BUILD_TYPE} mode")

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
        message(STATUS "${TESTS_UDP_SERVER} :: Compile Clang")
        message(STATUS "${TESTS_UDP_SERVER} :: ${CMAKE_BUILD_TYPE} mode")
        add_compile_options(-Wshadow)
    endif()
endif()

#
# Windows OS (MSVC)
#
if (WIN32)
    message(STATUS "${TESTS_UDP_SERVER} :: OS Win32")
    if (MSVC)
        message(STATUS "${TESTS_UDP_SERVER} :: microsoft visual studio compiler")
        message(STATUS "${TESTS_UDP_SERVER} :: ${CMAKE_BUILD_TYPE} mode")

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

# Build Unit Tests
file(GLOB UNIT_TEST_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "unit/*.cpp")
set( GTEST_INC_DIR "$ENV{deps_path}/out/include" )
set( GTEST_LIB_DIR "$ENV{deps_path}/out/lib" )
foreach(SOURCE_FILE ${UNIT_TEST_SRC})
    get_filename_component(EXE_NAME ${SOURCE_FILE} NAME_WE)
    set(EXE_NAME "unit_test_${EXE_NAME}")
    add_executable(${EXE_NAME} ${SOURCE_FILE})
    set_target_properties(${EXE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    target_include_directories(${EXE_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GTEST_INC_DIR}
    )
    # COMAND POST-BUILD
    add_custom_command(TARGET ${EXE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{bin_path}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${EXE_NAME}>"
            "$ENV{bin_path}/${EXE_NAME}"
        COMMENT "Copying ${EXE_NAME} to $ENV{bin_path} directory... Done!"
    )
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
    target_link_libraries(${EXE_NAME} PRIVATE
        ${GTEST_LIB_FILE}
        ${GTEST_MAIN_FILE}
    )
    message(STATUS "Configured example executable: bin/${EXE_NAME}")
endforeach()

# Integration Build
file(GLOB INTEGRATION_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "integration/*.cpp")
foreach(SOURCE_FILE ${INTEGRATION_SRC})
     get_filename_component(EXE_NAME ${SOURCE_FILE} NAME_WE)
     set(EXE_NAME "integration_${EXE_NAME}")
     add_executable(${EXE_NAME} ${SOURCE_FILE})
     set_target_properties(${EXE_NAME} PROPERTIES
         RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
     )
     # COMAND POST-BUILD
     add_custom_command(TARGET ${EXE_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{bin_path}"
         COMMAND ${CMAKE_COMMAND} -E copy_if_different
             "$<TARGET_FILE:${EXE_NAME}>"
             "$ENV{bin_path}/${EXE_NAME}"
         COMMENT "Copying ${EXE_NAME} to $ENV{bin_path} directory... Done!"
     )
     message(STATUS "Configured example executable: bin/${EXE_NAME}")
 endforeach()

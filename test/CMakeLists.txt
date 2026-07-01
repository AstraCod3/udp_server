#
# WINDOWS :
#
# > cmake .. -G "MinGW Makefiles"
#
# > cmake --build .
#
# OR
#
# > cmake .. -G "Visual Studio 17 2022" -A x64
#
# > cmake --build . --config Release
#
# LINUX :
#
# $ cmake .. && make
#
# debug :
#
# $ valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./std_thread_bin
#
# imposta una versione CMake minima necessaria per valutare lo script corrente.
#

cmake_minimum_required(VERSION 3.31.5)

set(PRJECT_NAME "UnitTest")

project(${PRJECT_NAME}
    VERSION 0.0.1
    DESCRIPTION "a unit test udp server base class"
    LANGUAGES CXX )

#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Windows OS
if (WIN32)
    message(STATUS "${PRJECT_NAME} :: OS Win32")
    if (MSVC)
        message(STATUS "${PRJECT_NAME} :: microsoft visual studio compiler")
        #
        ##if(CMAKE_BUILD_TYPE STREQUAL "debug" )
        ##    message(STATUS "${PRJECT_NAME} :: debug mode")
        ##elseif(CMAKE_BUILD_TYPE STREQUAL "release" )
        ##    message(STATUS "${PRJECT_NAME} :: release mode")
        ##endif()
        #
    endif (MSVC)

    if( MINGW)
       message(STATUS "${PRJECT_NAME} :: mingw compiler")

       #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -O3 -g3 -pedantic -Wall -Wextra")
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g3 -pedantic -Wall -Wextra")
    endif()

endif (WIN32)


#
# Linux OS
if (UNIX) 
    message(STATUS "${PROJ_MULTIOS} :: OS Unix")
    # if check compiler
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -O3 -g3 -pedantic -Wall -Wextra")
    #
    # debug
    if(CMAKE_BUILD_TYPE STREQUAL "debug" )
        message(STATUS "${PROJ_MULTIOS} :: debug mode")        
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG -ggdb")
    elseif(CMAKE_BUILD_TYPE STREQUAL "release" )
        message(STATUS "${PROJ_MULTIOS} :: release mode")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRELEASE")
    endif()

endif (UNIX)

message("-- CMAKE_SYSTEM_INFO_FILE: ${CMAKE_SYSTEM_INFO_FILE}")
message("-- CMAKE_SYSTEM_NAME:      ${CMAKE_SYSTEM_NAME}")
message("-- CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message("-- CMAKE_SYSTEM:           ${CMAKE_SYSTEM}")

set(UNITTEST_SRC 
     UnitTest.cpp
     ../UdpServer/BaseUdpServer.cpp )

set(UNITTEST_BIN unit_test_bin)

add_executable(${UNITTEST_BIN} ${UNITTEST_SRC})

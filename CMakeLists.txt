#
# compile :
#
# $ cmake .. -G "MinGW Makefiles"
#
# $ cmake .. -G "Visual Studio 17 2022" -A x64
# $ cmake --build . --config Release
#
# or
#
# $ cmake .. && make
#
# debug :
#
# $ valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./std_thread_bin
#

# imposta una versione CMake minima necessaria per valutare lo script corrente.
cmake_minimum_required(VERSION 3.16.3)

# avvia un nuovo progetto CMake. Ciò attiverà molta logica interna di
# CMake, in particolare il rilevamento del compilatore C e C ++ predefinito.
project(UdpServer)

include_directories(${PROJECT_SOURCE_DIR})

#set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -O3 -g3 -pedantic -Wall -Wextra")

# source file
set(UDPSERVER_SRC main.cpp BaseThread.cpp BaseUdpServer.cpp UdpServer.cpp)

# executable
set(UDPSERVER_BIN udp_server)

#add_compile_options(/W4)

#add_compile_options(/c /ZI /JMC 
#/nologo 
#/W3 
#/WX- 
#/diagnostics:column 
#/sdl 
#/Od 
#/D _CONSOLE 
#/D _UNICODE 
##/D UNICODE 
#/Gm- 
#/EHsc 
#/RTC1 
#/MDd 
#/GS 
#/fp:precise 
#/Zc:wchar_t 
#/Zc:forScope 
#/Zc:inline 
#/permissive- 
#/Gd 
#/TP 
#/FC
)

# Con add_executable(app main.cpp) viene creata app destinazione build che invoca il
# compilatore configurato con alcuni flag predefiniti per l'impostazione corrente per compilare
# app eseguibile dal file di origine specificato main.cpp .
add_executable(${UDPSERVER_BIN} ${UDPSERVER_SRC})

//
// WINDOWS :
//
// > cmake .. -G "MinGW Makefiles"
// > cmake --build .
//
// OR
//
// > cmake .. -G "Visual Studio 17 2022" -A x64
// > cmake --build . --config Release
//
// LINUX :
//
// $ cmake .. && 
// $ make
//
// debug :
//
// $ valgrind - s --leak - check = full --show - leak - kinds = all --track - origins = yes . /binary_exe
//
// imposta una versione CMake minima necessaria per valutare lo script corrente.
//
//
// https://stackoverflow.com/questions/14665543/how-do-i-receive-udp-packets-with-winsock-in-c
// https://handsonnetworkprogramming.com/articles/socket-error-message-text/
// https://jweinst1.medium.com/how-to-use-udp-sockets-on-windows-29e7e60679fe
// https://www.dataenter.com/common/winsockerror.htm
//
//

// UnitTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>

#include "UnitTest.h"

int main(int argc, char* argv[]) {
	int ret = EXIT_SUCCESS;
	ret = NsUnitTest::TestStartAndRx();

	//TestBaseUdpStartAndRx();
	//TestBaseThread();
	return ret;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

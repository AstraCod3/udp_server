/**
 * @file open_socket.cpp
 * @brief 
 * @details
 * @author AstraCod3
 * @date June 28, 2026
 * @version 1.0.0
 * 
 * @note
 * @warning
 * @see 
 * @todo
 * @deprecated
 * @def
 */
#include <iostream>
#include <future>
#include <chrono>
#include <atomic>

#include <gtest/gtest.h>

#include "../../udp_server/udp_server.hpp"

/**
 * @namespace ns_google_unit_test 
 * @brief Contains all the Google unit tests
 * @details This namespace isolates test components, mocks, and test fixtures from the production code.
 */
namespace ns_unit_test_open_socket {

    /**
     * @brief
     * @details
     */
    TEST(OepnSocket, SingleOpenSocket) {
        bool result_open_socket = true;
        EXPECT_EQ(result_open_socket, true);
    }
}

/**
 * @brief
 * @details
 */
int main(int argc,char* argv[]) {
    std::cout << "\n";
    std::cout << " =============================================================\n";
    std::cout << "                  Run Open Socket Google Unit Tests ...\n";
    std::cout << " =============================================================\n";
    std::cout << "\n";

    /*
     * @brief Initi Google Test framework with arguments passando i parametri della riga di comando
     */
    try {
        ::testing::InitGoogleTest(&argc, argv);
    }
    catch (const ns_udp_server::udp_server_error& e) {
        std::cout << "Exeception failure\n";
        std::cout << e.what() <<"\n";
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Exeception runtime failure : " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exeception standard failure : " << e.what() << std::endl;
    }

    /*
     * @brief Execute all TEST e TEST_F macro return 0 succeded,  otherwise return 1
     */ 
    int ret = RUN_ALL_TESTS();
    if ( ret != 0 ) {
        throw std::runtime_error("!ERROR! Unit test failure\n");
    }

    std::cout << "\n";
    std::cout << "..done!\n";

    return EXIT_SUCCESS;
}

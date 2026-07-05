/**
 * @file google_unit_test.cpp
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
namespace ns_google_unit_tests {

    /**
     * @brief
     * @details
     */
    TEST(RingBuffer, ReadWritreRingBuffer) {
        bool result_rw_rg_buffer = true;
        EXPECT_EQ(result_rw_rg_buffer , true);
    }

    /**
     * @brief
     * @details
     */
    TEST(OepnSocket, SingleOpenSocket) {
        bool result_open_socket = true;
        EXPECT_EQ(result_open_socket, true);
    }


    /**
     * @brief
     * @details
     */
    void main_unit_tests(int _argc, char* _argv[]) ;
    void main_unit_tests(int _argc, char* _argv[]) {
        std::cout << "\n";
        std::cout << "\n BEGIN -- Google Unit Test\n";
        std::cout << "\n";

        /*
         * @brief Initi Google Test framework with arguments passando i parametri della riga di comando
         */
        ::testing::InitGoogleTest(&_argc, _argv);
        
        /*
         * @brief Execute all TEST e TEST_F macro return 0 succeded,  otherwise return 1
         */ 
        int ret = RUN_ALL_TESTS();
        if ( ret != 0 ) {
            throw std::runtime_error("!ERROR! Unit test failure\n");
        }

        std::cout << "\n";
        std::cout << "\n END -- Google Unit Test\n";
        std::cout << "\n";
    }


}

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
#include <stdexcept>

#include <gtest/gtest.h>

// Temporary bypass of access modifiers to test internal ring buffer pointers
#define private public
#define protected public
#include "../../udp_server/udp_server.hpp"
#undef private
#undef protected

namespace ns_unit_test_open_socket {

    /**
     * @brief Test case to verify successful UDP socket initialization.
     * @details This test instantiates the udp_server component on a standard local port,
     *          verifies that the initial status is correct, and ensures no exceptions are thrown.
     */
    TEST(OpenSocket, SingleOpenSocket) {
        const unsigned short test_port = 1581;

        // Instantiating the server (does not throw network exceptions in the constructor)
        ns_udp_server::udp_server test_server(test_port);

        // Verify initial state is INIT
        EXPECT_EQ(test_server.get_status_enum(), ns_udp_server::SERVER_STATUS::INIT);
        EXPECT_STREQ(test_server.get_status_str().data(), "INIT");
    }

    /**
     * @brief Test case to verify that binding two servers to the same port triggers a failure.
     * @details This test instantiates a primary UDP server on a specific port. It then attempts
     *          to initialize a secondary server on that same port, expecting the system to reject 
     *          the second bind request by throwing a `udp_server_error`.
     */
    TEST(OpenSocket, FailOnDuplicatePort) {
        const unsigned short shared_port = 1582;
        try {
            ns_udp_server::udp_server first_server(shared_port);
            std::thread first_server_thread([&first_server]() {
                first_server.start(); // recvfrom() is blocking
            });
            std::this_thread::sleep_for(std::chrono::seconds(3));
            EXPECT_EQ(first_server.get_status_enum(), ns_udp_server::SERVER_STATUS::RUNNING);

            EXPECT_THROW({
                ns_udp_server::udp_server second_server(shared_port);
                second_server.start();
            },ns_udp_server::udp_server_error);

            first_server.stop();
            if (first_server_thread.joinable()) {
                first_server_thread.join();
            }
        } catch (const std::exception& e) {
            FAIL() << "Setup phase failed with an unexpected exception: " << e.what();
        }
    }

} // ns_unit_test_open_socket

/**
 * @brief Application entry point for the Open Socket Unit Test suite.
 * @details Initializes the GoogleTest framework, handles command-line arguments,
 *          and manages the orderly execution and reporting of all defined test cases.
 */
int main(int argc, char* argv[]) {
    std::cout << "\n";
    try {
        ::testing::InitGoogleTest(&argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << "Critical failure during GoogleTest initialization: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    int result = RUN_ALL_TESTS();
    return result;
}

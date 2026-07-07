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

        // Extract data through the public official interface to verify initial clean state
        int current_err_code = 0;
        std::string current_err_msg = "";
        test_server.get_error(current_err_code, current_err_msg);

        // Assert that everything is cleared out out-of-the-box
        EXPECT_EQ(current_err_code, 0);
        EXPECT_TRUE(current_err_msg.empty());
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
            // Act 1: Initialize the first server on the designated port
            ns_udp_server::udp_server first_server(shared_port);

            std::thread server_thread([&first_server]() {
                first_server.start();
            });
            
            // Note: If your socket binding (bind_socket) happens inside start() 
            // and start() is blocking, you should spin it on a separate thread, e.g.:
            // std::thread s1_thread([&]() { first_server.start(); });
            // std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Let it bind
            
            // Act 2 & Assert: Attempting to bind a second server to the same port must fail
            // We use EXPECT_THROW to verify that the operation triggers your custom exception.
            EXPECT_THROW({
                ns_udp_server::udp_server second_server(shared_port);
                second_server.start();

                // If the bind logic is inside start(), call it here to trigger the failure:
                // second_server.start();
            }, ns_udp_server::udp_server_error);

            // Clean up the first thread if you started one:
            // first_server.stop();
            // if (s1_thread.joinable()) s1_thread.join();

        } catch (const std::exception& e) {
            FAIL() << "Setup phase failed with an unexpected exception: " << e.what();
        }
    }
}

/**
 * @brief Application entry point for the Open Socket Unit Test suite.
 * @details Initializes the GoogleTest framework, handles command-line arguments,
 *          and manages the orderly execution and reporting of all defined test cases.
 */
int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << " =============================================================\n";
    std::cout << "                  Run Open Socket Google Unit Tests ...\n";
    std::cout << " =============================================================\n";
    std::cout << "\n";

    try {
        ::testing::InitGoogleTest(&argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << "Critical failure during GoogleTest initialization: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    int result = RUN_ALL_TESTS();
    
    std::cout << "\n";
    std::cout << "..done!\n";
    return result;
}



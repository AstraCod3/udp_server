/**
 * @file test.cpp
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

#include <chrono>
#include <iostream>
#include <thread>

#include "../udp_server/udp_server.hpp"

using namespace std::chrono_literals;

namespace ns_example_udp_server {

        static constexpr unsigned short int lport = 1581; ///< Target UDP local port.
    
        // Explicit type declaration for the server instance
        ns_udp_server::udp_server udpsrv(lport); ///< Global or namespace-scoped server instance.

        bool is_running = false; ///< Execution flag ensuring safe thread lifetimes.

        /**
         * @brief Spawns the dedicated network background thread using a lambda expression.
         */
        void start_srv() {
            if (is_running)
                return;
            is_running = true;

            // Launch the network ingestion loop on a separate background thread
            ///< Active thread handle managing network ingestion.
            std::thread thr_udp_srv = std::thread( [&] () {
                // The thread continuously invokes our zero-copy recvfrom implementation
                std::cout << "UDP Server thread spawned successfully on port " << lport << std::endl;
                udpsrv.start(); 
            });
            
        }

        /**
         * @brief Standard thread routine executing utility delay operations.
         */
        void check_if_stop() {
            std::this_thread::sleep_for(500ms);
        }

        /**
         * @brief Signals the shutdown flag and safely synchronizes thread termination.
         */
        void stop_srv() {
            if (!is_running)
                return;
            is_running = false;

            // NOTE: If recvfrom is currently blocking, you should close the msocket descriptor 
            // here (using closesocket() on Windows) to force recvfrom to unblock instantly.

            if (thr_udp_srv.joinable()) {
                thr_udp_srv.join(); ///< Wait for the background loop to complete its current iteration and exit.
            }
             
            std::cout << "UDP Server thread joined and safely destroyed." << std::endl;
        }

        /**
         * @brief Orchestrator entry-point modeling execution flow, background tasks, and safe lifecycle termination.
         * @return int Status execution code (0 for success).
         */
        int example_main_udp_server() {
            std::cout << "\n";
            std::cout << "Running example_main_udp_server start ...\n";
            std::cout << "\n";
            // 1. Fire up the background server loop
            start_srv();

            // 2. Perform concurrent background application tasks while the network thread handles packet ingestion
            //oo(); 
            //td::cout << "Main Thread: Doing some work while server is receiving packets..." << std::endl;
            //jjfoo();

            // 3. Gracefully stop and join the network architecture before the scope closes
            // stop_srv();
            std::cout << "\n";
            std::cout << "...done!!\n";
            std::cout << "\n";
            return 0; ///< Safe entry point exit.
        }


}

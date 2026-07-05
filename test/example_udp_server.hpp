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
#include <string_view>
#include <condition_variable>
#include <string_view>

#include "../udp_server/udp_server.hpp"

//using std::chrono_literals;

namespace ns_example_udp_server {

    /**
     * @brief Orchestrator entry-point modeling execution flow, background tasks, and safe lifecycle termination.
     * @return int Status execution code (0 for success).
     */
    void example_main_udp_server() ;
    void example_main_udp_server() {
        std::cout << "\n";
        std::cout << "Running example_main_udp_server start ...\n";
        std::cout << "\n";
        
        unsigned short lport = 1581; ///< Target UDP local port.
        std::cout << " local port : " << lport << "\n";
        // Explicit type declaration for the server instance
        ns_udp_server::udp_server udpsrv(lport); ///< Global or namespace-scoped server instance.
        udpsrv.start(); 

        std::thread server_thread([&]() {
            std::cout << "[Thread] Server UDP start\n";
            udpsrv.start(); 
            std::cout << "\n";
        });


        // Perform concurrent background application tasks while the network thread handles packet ingestion
    
        int pack_counter = 0;
        const int pack_received = 10;

        std::vector<uint8_t> buff_rx;
        buff_rx.reserve( ns_udp_server::max_size_udp_rx );
        std::cout << "get_last_packet\n";

        while ( pack_counter < pack_received ) {
            udpsrv.get_last_packet( buff_rx );
            pack_counter++;
            // Crea una vista ASCII sul vettore esistente senza fare copie in memoria
            std::string_view ascii_view(reinterpret_cast<const char*>(buff_rx.data()), buff_rx.size());
            std::cout << " received packet : " << pack_counter << std::endl;
            // std::cout << " received packet : " << pack_counter << std::endl;
        }


        /* std::cout << " waiting 500ms\n";
        std::this_thread::sleep_for(500ms); */

        udpsrv.stop();
        
        if (server_thread.joinable()) {
            server_thread.join();
        }

        std::cout << "\n";
        std::cout << "...done!!\n";
        std::cout << "\n";
        // return 0; ///< Safe entry point exit.
    }
}
